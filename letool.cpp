// letool.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "lestructs.h"
#include <stdio.h>
#include <string.h>
#include <direct.h>
#include <inttypes.h>
#include <vector>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <options> <LE_file_path>\n", argv[0]);
        printf("  options :\n");
        printf("    -a : add space before enumerated data pages\n");
        //printf("    -b : add fixups entries");
        printf("  note : you can make fixup data file, to be applied, named fixup.txt");
        printf("         if no file found, dummy fixup size of 7, will be applid.");
        return 1;
    }

    // create a buffer to store the new file name
    char new_file_name[200];
    char full_input_file_name[200];
    char fixupdat_full_name[200];
    char* option = nullptr;
    char* inputfile_name = argv[2];
    char* cwd = _getcwd(NULL, 0);

    option = argv[1];

    if (cwd == nullptr)
    {
        std::cerr << "Error: Unable to get the current working directory." << std::endl;
        return 1;
    }

    // Copy the CWD to the buffer
    strcpy_s(new_file_name, 100, cwd);
    // Concatenate the original file name to the buffer
    strncat_s(new_file_name, 100, "\\", 1);
    // copy the prefix to the buffer
    if (strcmp(option, "-a") == 0) strncat_s(new_file_name, 100, "MOD_",4); else
        if (strncmp(option, "-b",2) == 0) strncat_s(new_file_name, 100, "MOD2_", 5); else
            strncat_s(new_file_name, 100, "MODunk_", 7);

    strncat_s(new_file_name, 100, inputfile_name, strlen(inputfile_name));

    // concatenate the original file name to the buffer
    strcpy_s(full_input_file_name, 100, cwd);
    strncat_s(full_input_file_name, 100, "\\", 1);
    strcat_s(full_input_file_name, 100, argv[2]);

    // concatenate the original file name to the buffer
    strcpy_s(fixupdat_full_name, 100, cwd);
    strncat_s(fixupdat_full_name, 100, "\\", 1);
    strcat_s(fixupdat_full_name, 100, "fixup.txt");

    // print the new file name
    printf("Original file name is %s\n", full_input_file_name);
    printf("The target file name is %s\n", new_file_name);
    printf("Fixup data file name is %s\n\n", fixupdat_full_name);

    free(cwd);

    if (option)
    {
        if (strcmp(option, "-a") == 0)
        {
            bool fixupdatok = false;
            FILE* fixupdatfile;
            errno_t fxdaterr = fopen_s(&fixupdatfile, fixupdat_full_name, "r");
            if (fxdaterr == 0 && fixupdatfile != NULL) fixupdatok = true;

            // Example: Process the -a option
            printf("Adding space before enumerated data pages...\n");
            
            FILE* infile;
            if (fopen_s(&infile, full_input_file_name, "rb") != 0) 
            {
                perror("Error opening input file");
                return 1;
            }

            // Seek to the LE header offset (0x3C)
            fseek(infile, 0x3C, SEEK_SET);
            uint16_t leHeaderOffset;
            fread(&leHeaderOffset, sizeof(uint16_t), 1, infile);
            //printf("LE header offset: 0x%x\n", leHeaderOffset);

            // Seek to the LE header location
            fseek(infile, leHeaderOffset, SEEK_SET);

            // Read and parse the LE header
            LEHeader leHeader;
            fread(&leHeader, sizeof(LEHeader), 1, infile);

            // Check the LE signature to verify it's a valid LE file
            if (leHeader.signature != 0x454C) 
            {
                printf("Not a valid LE file.\n");
                fclose(infile);
                return 1;
            }

            printf("data_pages_offset : 0x%X (1st page offset)\n", leHeader.data_pages_offset);

            const size_t insertionSize = 0x1000;
            static char insertionData[insertionSize];  // An array of 0x00 bytes

            memset(insertionData, 0x00, insertionSize);

            // Calculate the size of data to copy before data_pages_offset
            const size_t copySize = leHeader.data_pages_offset;

            // Open the target file for both reading and writing
            FILE* targetFile;
            if (fopen_s(&targetFile, new_file_name, "wb") != 0)
            {
                perror("Error opening target file");
                fclose(infile);  // Close the input file
                return 1;
            }

            // Seek to the beginning of the target file
            fseek(targetFile, 0, SEEK_SET);
            fseek(infile, 0, SEEK_SET);

            // Create a buffer for reading and writing data
            const size_t bufferSize = 4096;  // Adjust the buffer size as needed
            static char buffer[bufferSize];

            memset(buffer, 0xFF, bufferSize);

            // Copy data from the input file to the target file in chunks
            size_t totalBytesCopied = 0;

            while (totalBytesCopied < copySize)
            {
                size_t bytesToCopy = std::min(copySize - totalBytesCopied, bufferSize);
                size_t bytesRead = fread(buffer, 1, bytesToCopy, infile);

                if (bytesRead == 0)
                {
                    perror("Error reading data from the input file");
                    fclose(targetFile);
                    fclose(infile);  // Close the input file
                    return 1;
                }

                size_t bytesWritten = fwrite(buffer, 1, bytesRead, targetFile);

                if (bytesWritten != bytesRead)
                {
                    perror("Error writing data to the target file");
                    fclose(targetFile);
                    fclose(infile);  // Close the input file
                    return 1;
                }

                totalBytesCopied += bytesRead;
            }

            // After copying the data, write insertionData (this space is for header/fixup editing) 
            size_t bytesWritten = fwrite(insertionData, 1, insertionSize, targetFile);

            if (bytesWritten != insertionSize)
            {
                perror("Error writing insertion data to the target file");
                fclose(targetFile);
                fclose(infile);  // Close the input file
                return 1;
            }

            // Copy the data from data_pages_offset to the end of the infile to targetFile

            // calculate offset of additional page
            // TODO : complete totaladditionalpage changes feature
            const int totaladditionalpage = 1;
            fseek(infile, leHeader.object_table_offset + leHeaderOffset, SEEK_SET);
            ObjectTable objTable;
            fread(&objTable, sizeof(ObjectTable), 1, infile);
            long additionpageofs = (objTable.numberOfPageTableEntries * leHeader.memory_page_size) + leHeader.data_pages_offset;
            //printf("additionpageofs = %x\n", additionpageofs);

            // Seek to the position data_pages_offset in the infile
            fseek(infile, leHeader.data_pages_offset, SEEK_SET);

            while (1)
            {
                if (ftell(infile) == additionpageofs)
                {
                    // insert additional page at the end of code section
                    memset(buffer, 0, bufferSize);
                    for (int i = 0; i < totaladditionalpage; i++)
                    {
                        fwrite(buffer, 1, bufferSize, targetFile);
                    }
                }

                size_t bytesRead = fread(buffer, 1, bufferSize, infile);
                if (bytesRead == 0)
                {
                    if (feof(infile))
                    {
                        break;  // Reached the end of the infile
                    }
                    else
                    {
                        perror("Error reading data from the input file");
                        fclose(targetFile);
                        fclose(infile);
                        return 1;
                    }
                }

                size_t bytesWritten = fwrite(buffer, 1, bytesRead, targetFile);
                if (bytesWritten != bytesRead)
                {
                    perror("Error writing data to the target file");
                    fclose(targetFile);
                    fclose(infile);
                    return 1;
                }
            }

            //update object table

            fseek(infile, leHeader.object_table_offset + leHeaderOffset, SEEK_SET);
            fseek(targetFile, leHeader.object_table_offset + leHeaderOffset, SEEK_SET);

            uint32_t totalobj = leHeader.object_table_entries;
            uint32_t addedpagenumber = 0;
            for (uint32_t i = 0; i < totalobj; i++)
            {
                fread(&objTable, sizeof(ObjectTable), 1, infile);
                if (i == 0) 
                {
                    objTable.numberOfPageTableEntries += totaladditionalpage; 
                    addedpagenumber = objTable.numberOfPageTableEntries;
                    objTable.virtualMemorySize += 0x1000;
                }
                else
                {
                    objTable.objectPageTableIndex++;
                    objTable.relocationBaseAddress += (totaladditionalpage * 0x1000);
                }
                    
                fwrite(&objTable, sizeof(ObjectTable), 1, targetFile);
            }

            if (ftell(infile) != (leHeader.object_page_map_offset + leHeaderOffset))
            {
                printf("warning! object page map does not follow directly after object table.\n");
                fclose(targetFile);
                fclose(infile);
                return 1;
            }

            //update object page map
            fseek(infile, leHeader.object_page_map_offset + leHeaderOffset, SEEK_SET);
            fseek(targetFile, leHeader.object_page_map_offset + leHeaderOffset, SEEK_SET);
            uint16_t totalpagemap_old_entries = leHeader.num_memory_pages;
            
            int pmapsz = (totalpagemap_old_entries + 1) * sizeof(ObjectPageMapEntries);
            std::vector<char> pmapentries(pmapsz);
            fread(pmapentries.data(), sizeof(ObjectPageMapEntries), totalpagemap_old_entries, infile);
            pmapentries[pmapsz - 2] = (char)(totalpagemap_old_entries + 1);
            fwrite(pmapentries.data(), sizeof(ObjectPageMapEntries), totalpagemap_old_entries + 1, targetFile);

            if (leHeader.resource_table_offset != leHeader.resident_names_table_offset)
            {
                printf("warning! resource table offset is not equal to resident name table offset.\n");
                fclose(targetFile);
                fclose(infile);
                return 1;
            }

            if (ftell(infile) != (leHeader.resource_table_offset + leHeaderOffset))
            {
                printf("warning! resident name table offset did not follow after object page map.\n");
                fclose(targetFile);
                fclose(infile);
                return 1;
            }

            //copy resident name data to targetfile
            if (leHeader.fixup_page_table_offset - leHeader.entry_table_offset != 1)
            {
                printf("warning! entry table is not empty.\n");
                fclose(targetFile);
                fclose(infile);
                return 1;
            }
            
            uint32_t residentnm_datalen = (leHeader.entry_table_offset + leHeaderOffset) - ftell(infile);
            uint32_t updatedresident_names_table_offset = ftell(targetFile) - leHeaderOffset; //remember write 2x later, resident name and resource table

            std::vector<char> residentnamedata(residentnm_datalen);
            fread(residentnamedata.data(), 1, residentnm_datalen, infile);
            fwrite(residentnamedata.data(), 1, residentnm_datalen, targetFile);

            //copy entry table data, which is just one byte of 00
            const char entrytabledata = 0;
            uint32_t updatedentry_table_offset = ftell(targetFile) - leHeaderOffset;
            fwrite(&entrytabledata, 1, 1, targetFile);
            char tobediscard;
            fread(&tobediscard, 1, 1, infile); //move the file pointer accordingly

            //update fixup page table
            //assume a dummy size of added page fixup is just 1 simple type fixup, which is 7 bytes
            size_t dummyfixupsz = 7;

            if (fixupdatok)
            {
                char tempbuf[20];
                fseek(fixupdatfile, 0, SEEK_SET);
                dummyfixupsz = 0;
                while (fgets(tempbuf, sizeof(tempbuf), fixupdatfile) != NULL)
                {
                    size_t datlen = strlen(tempbuf);
                    if ((datlen == 15) || (datlen == 19)) --datlen;
                    if (datlen != 14)
                        if (datlen != 18)
                            printf("warning! something wrong with size check of the fixup data!");
                    dummyfixupsz += (datlen / 2);
                }
            }
            printf("planned fixup size is 0x%x\n", (int)dummyfixupsz);

            uint32_t oldfixuppagetablesz = leHeader.fixup_record_table_offset - leHeader.fixup_page_table_offset;
            //printf("oldfixuppagetablesz = %x\n", oldfixuppagetablesz);
            //printf("# of oldfixuppage = %x\n", oldfixuppagetablesz/4);
            std::vector<uint32_t> oldfixuppagetabledata((oldfixuppagetablesz/4)); 
            std::vector<uint32_t> newfixuppagetabledata((oldfixuppagetablesz/4) + 1);
            fread(oldfixuppagetabledata.data(), sizeof(uint32_t), oldfixuppagetablesz/4, infile);
            for (uint32_t i = 0; i < ((oldfixuppagetablesz / 4) + 1); i++)
            {
                if (i >= addedpagenumber)
                {
                    newfixuppagetabledata[i] = oldfixuppagetabledata[i - 1] + (uint32_t)dummyfixupsz;
                }
                else
                    newfixuppagetabledata[i] = oldfixuppagetabledata[i];
            }
            uint32_t newfixuprecordsz = newfixuppagetabledata[(oldfixuppagetablesz / 4)];
            uint32_t oldfixuprecordsz = oldfixuppagetabledata[(oldfixuppagetablesz / 4) - 1];

            printf("old fixup record size = %x\n", oldfixuprecordsz);
            printf("new fixup record size = %x\n", newfixuprecordsz);
            uint32_t updatedfixup_page_table_offset = ftell(targetFile) - leHeaderOffset;
            fwrite(newfixuppagetabledata.data(), sizeof(uint32_t), (oldfixuppagetablesz/4) + 1, targetFile);

            //copy and update fixup records
            uint32_t updatedfixup_record_table_offset = ftell(targetFile) - leHeaderOffset;
            fseek(infile, leHeader.fixup_record_table_offset + leHeaderOffset, SEEK_SET);

            const size_t fxbufferSize = 0x2000;  // Adjust the buffer size as survey result, i.e. you need to find the biggest one
            static char fxbuffer[fxbufferSize];
 
            for (uint32_t i = 0; i < ((oldfixuppagetablesz / 4)); i++)
            {
                if ((i != ((oldfixuppagetablesz / 4)-1)) && (i != addedpagenumber))
                {
                    size_t sizetoread = oldfixuppagetabledata[i + 1] - oldfixuppagetabledata[i];
                    if (sizetoread > fxbufferSize)
                    {
                        printf("warning! fxbuffer is not big enough, it should be greater than %x\n", (int)sizetoread);
                        fclose(targetFile);
                        fclose(infile);
                        return 1;
                    }
                    //printf("%x : sizetoread = %x, infile offset = %x\n", i + 1, (int)sizetoread, ftell(infile));
                    //printf("%x # %x\n",i, (int)sizetoread);
                    if (sizetoread != 0)
                    {
                        //printf("reading, then write to %x\n", ftell(targetFile));
                        size_t readresult = fread(fxbuffer, 1, sizetoread, infile);
                        //printf("readresult = %x\n", (int)readresult);
                        fwrite(fxbuffer, 1, sizetoread, targetFile);
                    }
                }
                if (i == addedpagenumber)
                {
                    printf("dummy fixup record offset is at %x\n", ftell(targetFile));
                    if (fixupdatok == false)
                    {
                        char dummybuf[7];
                        memset(dummybuf, 0xAA, 7);
                        fwrite(dummybuf, 1, 7, targetFile);
                    }
                    else
                    {
                        char dummy2buf[20];
                        char dummy2binbuf[20];
                        fseek(fixupdatfile, 0, SEEK_SET);
                        while (fgets(dummy2buf, sizeof(dummy2buf), fixupdatfile) != NULL)
                        {
                            size_t datlen = strlen(dummy2buf);
                            for (size_t i = 0; i < datlen; i += 2) {
                                char hexByte[3] = { dummy2buf[i], dummy2buf[i + 1], '\0' };
                                dummy2binbuf[i / 2] = (unsigned char)strtol(hexByte, NULL, 16);
                            }
                            if ((datlen == 15) || (datlen == 19)) --datlen;
                            if (datlen != 14)
                                if (datlen != 18)
                                    printf("warning! something wrong with the size of the fixup data!");
                            fwrite(dummy2binbuf, 1, datlen/2, targetFile);
                            printf("writing %d bytes fixup record..\n", (int)datlen / 2);
                        }
                    }
                }
            }

            printf("last offset is at %x\n", ftell(targetFile));
            uint32_t updatedimported_modules_name_table_offset = ftell(targetFile) - leHeaderOffset;

            // Seek to the position of leHeader in the target file
            fseek(infile, leHeaderOffset, SEEK_SET);

            // Read the existing leHeader from the target file
            LEHeader updatedLeHeader;
            fread(&updatedLeHeader, sizeof(LEHeader), 1, infile);

            // Write the updated leHeader structure back to the target file
            updatedLeHeader.fixup_record_table_offset = updatedfixup_record_table_offset;
            updatedLeHeader.fixup_page_table_offset = updatedfixup_page_table_offset;
            updatedLeHeader.entry_table_offset = updatedentry_table_offset;
            updatedLeHeader.resident_names_table_offset = updatedresident_names_table_offset;
            updatedLeHeader.resource_table_offset = updatedresident_names_table_offset;
            updatedLeHeader.num_memory_pages += 1;
            updatedLeHeader.data_pages_offset += insertionSize;
            updatedLeHeader.imported_modules_name_table_offset = updatedimported_modules_name_table_offset;
            updatedLeHeader.imported_procedure_name_table_offset = updatedimported_modules_name_table_offset;
            updatedLeHeader.fixup_section_size = updatedimported_modules_name_table_offset - updatedfixup_page_table_offset + 1;
            updatedLeHeader.loader_section_size = updatedimported_modules_name_table_offset - leHeader.object_table_offset + 1;

            fseek(targetFile, leHeaderOffset, SEEK_SET);
            bytesWritten = fwrite(&updatedLeHeader, sizeof(LEHeader), 1, targetFile);

            // Close the target file
            fclose(targetFile);

            // Close the input file
            fclose(infile);

            if (fixupdatok) fclose(fixupdatfile);

            // Rest of your code...
            printf("adding Done.");
            return 0;

        }

        if (strcmp(option, "-b") == 0)
        {
            // Extract the number that follows -b
            int num, num2, newfixupRecordsz;
            printf("Enter the number of (internal ref = 00h) fixups entries : ");
            if (scanf_s("%d", &num) != 1)
            {
                printf("Invalid input. Please specify a number.\n");
                return 1;
            }
            printf("Enter the number of (32-bit TRGOFF = 10h) fixups entries : ");
            if (scanf_s("%d", &num2) != 1)
            {
                printf("Invalid input. Please specify a number.\n");
                return 1;
            }
            newfixupRecordsz = (num * 7) + (num2 * 9);

            FILE* infile;
            FILE* targetFile;

            // Open infile for reading
            if (fopen_s(&infile, full_input_file_name, "rb") != 0)
            {
                perror("Error opening input file");
                return 1;
            }

            // Open targetFile for writing (create if it does not exist)
            if (fopen_s(&targetFile, new_file_name, "wb") != 0)
            {
                perror("Error opening target file");
                fclose(infile);  // Close the input file
                return 1;
            }

            // Create a buffer for reading and writing data
            const size_t bufferSize = 4096;  // Adjust the buffer size as needed
            char buffer[bufferSize];

            // Copy data from infile to targetFile in chunks
            while (1)
            {
                size_t bytesRead = fread(buffer, 1, bufferSize, infile);
                if (bytesRead == 0)
                {
                    if (feof(infile))
                    {
                        break;  // Reached the end of infile
                    }
                    else
                    {
                        perror("Error reading data from the input file");
                        fclose(targetFile);
                        fclose(infile);
                        return 1;
                    }
                }

                size_t bytesWritten = fwrite(buffer, 1, bytesRead, targetFile);
                if (bytesWritten != bytesRead)
                {
                    perror("Error writing data to the target file");
                    fclose(targetFile);
                    fclose(infile);
                    return 1;
                }
            }

            //update fixup record table
            std::vector<char> insertionData(newfixupRecordsz);
            memset(insertionData.data(), 0x00, newfixupRecordsz);

            //update header


            // Close both files
            fclose(targetFile);
            fclose(infile);

            return 0;

        }

        // Add more options processing here
    }


    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
