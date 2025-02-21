/******************************************************************************
 * Minimal FAT12 Reader in C
 *
 * Usage:
 *   ./fat12read <disk image> <filename_in_8.3_format>
 *
 * Description:
 *   - Reads the boot sector (BPB).
 *   - Loads the FAT.
 *   - Loads the root directory.
 *   - Finds a given file by comparing its 11-byte (8.3) name.
 *   - Follows the FAT chain to read the file into memory.
 *   - Prints readable characters to stdout, otherwise prints them as <XX>.
 *
 * Example:
 *   ./fat12read floppy.img KERNEL  BIN
 ******************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Define a 'bool' type for convenience */
typedef uint8_t bool;
#define true 1
#define false 0

/* ------------------------------------------------------------------------- */
/* Structures for the Boot Sector (BPB) and Directory Entry (packed structs) */
/* ------------------------------------------------------------------------- */

#pragma pack(push, 1)  // Ensure no padding between members

/* FAT12 Boot Sector (BPB + EBPB) */
typedef struct 
{
    uint8_t BootJumpInstruction[3];  // e.g., 0xEB 0x58 0x90
    uint8_t OemIdentifier[8];        // e.g., "MSWIN4.1"

    uint16_t BytesPerSector;         // Typically 512 for floppies
    uint8_t  SectorsPerCluster;      // Usually 1 for a 1.44MB floppy
    uint16_t ReservedSectors;        // Usually 1
    uint8_t  FatCount;               // # of FAT copies (2)
    uint16_t DirEntryCount;          // e.g., 224 for 1.44MB
    uint16_t TotalSectors;           // 2880 for 1.44MB
    uint8_t  MediaDescriptorType;    // 0xF0
    uint16_t SectorsPerFat;          // 9 for 1.44MB
    uint16_t SectorsPerTrack;        // 18
    uint16_t Heads;                  // 2
    uint32_t HiddenSectors;          // 0 on a floppy
    uint32_t LargeSectorCount;       // Usually 0 on a 1.44MB floppy

    // Extended BPB fields
    uint8_t  DriveNumber;            // 0x00 (floppy) or 0x80 (HDD)
    uint8_t  _Reserved;              // Reserved byte
    uint8_t  Signature;              // 0x29 indicates an EBPB
    uint32_t VolumeId;               // Volume serial number
    uint8_t  VolumeLabel[11];        // e.g., "NO NAME    "
    uint8_t  SystemId[8];            // e.g., "FAT12   "

} BootSector;

/* FAT12 (or FAT16) directory entry */
typedef struct 
{
    uint8_t  Name[11];       // 8 chars for name + 3 for extension = 11
    uint8_t  Attributes;     // File attributes
    uint8_t  _Reserved;
    uint8_t  CreatedTimeTenths;
    uint16_t CreatedTime;
    uint16_t CreatedDate;
    uint16_t AccessedDate;
    uint16_t FirstClusterHigh; // FAT16 only, FAT12 doesn't really use
    uint16_t ModifiedTime;
    uint16_t ModifiedDate;
    uint16_t FirstClusterLow;  // For FAT12, all cluster info is here
    uint32_t Size;             // File size in bytes
} DirectoryEntry;

#pragma pack(pop)

/* ------------------------------------------------------------------------- */
/* Global Variables: pointers to structures in memory after reading disk data */
/* ------------------------------------------------------------------------- */

BootSector      g_BootSector;        // We'll store the BPB/EBPB here
uint8_t*        g_Fat           = NULL; // Pointer to a buffer holding the FAT
DirectoryEntry* g_RootDirectory = NULL; // Pointer to a buffer for the root dir
uint32_t        g_RootDirectoryEnd = 0; // LBA where data area starts

/* ------------------------------------------------------------------------- */
/* Helper Functions for Reading the Disk Image                                */
/* ------------------------------------------------------------------------- */

/* Read the boot sector from the disk image file */
bool readBootSector(FILE* disk)
{
    /* The BPB is exactly sizeof(BootSector) bytes. */
    return (fread(&g_BootSector, sizeof(g_BootSector), 1, disk) == 1);
}

/* Read 'count' sectors from LBA 'lba' into 'bufferOut' */
bool readSectors(FILE* disk, uint32_t lba, uint32_t count, void* bufferOut)
{
    /* Move file pointer: LBA * BytesPerSector */
    if (fseek(disk, lba * g_BootSector.BytesPerSector, SEEK_SET) != 0)
        return false;

    /* Read 'count' sectors. Each sector has BytesPerSector bytes */
    size_t readCount = fread(bufferOut, g_BootSector.BytesPerSector, count, disk);
    if (readCount != count)
        return false;

    return true;
}

/* Read the first FAT from the disk image */
bool readFat(FILE* disk)
{
    uint32_t fatSizeBytes = g_BootSector.SectorsPerFat * g_BootSector.BytesPerSector;
    g_Fat = (uint8_t*) malloc(fatSizeBytes);
    if (!g_Fat) return false;

    /* The FAT starts at LBA = ReservedSectors. We read 'SectorsPerFat' sectors. */
    return readSectors(disk, g_BootSector.ReservedSectors, g_BootSector.SectorsPerFat, g_Fat);
}

/* Read the root directory from the disk image */
bool readRootDirectory(FILE* disk)
{
    /* Root directory starts right after all the FAT copies */
    uint32_t rootDirLba = g_BootSector.ReservedSectors
                        + g_BootSector.SectorsPerFat * g_BootSector.FatCount;

    /* Root directory size in bytes = #Entries * 32 (sizeof(DirectoryEntry)) */
    uint32_t rootDirByteSize = g_BootSector.DirEntryCount * sizeof(DirectoryEntry);

    /* Convert rootDirByteSize to a number of sectors (round up if needed) */
    uint32_t sectors = rootDirByteSize / g_BootSector.BytesPerSector;
    if ((rootDirByteSize % g_BootSector.BytesPerSector) != 0)
        sectors++;

    /* We'll need this to know where the data area starts (LBA) = end of root dir */
    g_RootDirectoryEnd = rootDirLba + sectors;

    /* Allocate enough space to store the entire root directory */
    g_RootDirectory = (DirectoryEntry*) malloc(sectors * g_BootSector.BytesPerSector);
    if (!g_RootDirectory) return false;

    /* Read the actual root directory */
    return readSectors(disk, rootDirLba, sectors, g_RootDirectory);
}

/* Search the root directory for a file matching the 8.3 name in 'name' (11 chars) */
DirectoryEntry* findFile(const char* name11)
{
    /* Compare with each of the 11-byte directory names in the root directory */
    for (uint32_t i = 0; i < g_BootSector.DirEntryCount; i++)
    {
        if (memcmp(name11, g_RootDirectory[i].Name, 11) == 0)
        {
            return &g_RootDirectory[i];
        }
    }
    return NULL;
}

/* Read a file by following the FAT12 cluster chain */
bool readFile(DirectoryEntry* fileEntry, FILE* disk, uint8_t* outputBuffer)
{
    bool ok = true;
    uint16_t currentCluster = fileEntry->FirstClusterLow;

    /* We'll keep reading cluster-by-cluster until we hit an end-of-chain marker. */
    do {
        /* Convert cluster -> LBA. 
         * For FAT12, the data area starts at g_RootDirectoryEnd.
         * Each cluster is SectorsPerCluster sectors.
         * Cluster #2 starts at that LBA, so for cluster N: LBA = start + (N-2)*SectorsPerCluster.
         */
        uint32_t lba = g_RootDirectoryEnd
                     + (currentCluster - 2) * g_BootSector.SectorsPerCluster;

        /* Read the cluster from disk into 'outputBuffer' */
        ok = ok && readSectors(disk, lba, g_BootSector.SectorsPerCluster, outputBuffer);
        outputBuffer += g_BootSector.SectorsPerCluster * g_BootSector.BytesPerSector;

        /* Now figure out the next cluster by looking at the FAT.
         * FAT12 uses 12 bits per entry.  
         * - If cluster is even, we use the low 12 bits of the 16 bits at 'fatIndex'.
         * - If cluster is odd, we use the high 12 bits.
         */
        uint32_t fatIndex = currentCluster * 3 / 2; // Byte offset in FAT
        if ((currentCluster % 2) == 0)
        {
            /* Even cluster -> bits [11..0] of that 16-bit chunk */
            currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) & 0x0FFF;
        }
        else
        {
            /* Odd cluster -> bits [15..4] of that 16-bit chunk */
            currentCluster = (*(uint16_t*)(g_Fat + fatIndex)) >> 4;
        }

    } while (ok && currentCluster < 0x0FF8);

    return ok;
}

/* ------------------------------------------------------------------------- */
/* main: usage: ./fat12read <disk image> <8.3 filename>                     */
/* ------------------------------------------------------------------------- */
int main(int argc, char** argv)
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <disk_image> <filename_8.3>\n", argv[0]);
        return 1;
    }

    /* Open disk image in read-binary mode */
    FILE* disk = fopen(argv[1], "rb");
    if (!disk) {
        fprintf(stderr, "Error: Cannot open disk image '%s'\n", argv[1]);
        return 2;
    }

    /* 1) Read boot sector (BPB) */
    if (!readBootSector(disk)) {
        fprintf(stderr, "Error: Could not read boot sector!\n");
        fclose(disk);
        return 3;
    }

    /* 2) Read first FAT */
    if (!readFat(disk)) {
        fprintf(stderr, "Error: Could not read FAT!\n");
        fclose(disk);
        return 4;
    }

    /* 3) Read root directory */
    if (!readRootDirectory(disk)) {
        fprintf(stderr, "Error: Could not read root directory!\n");
        free(g_Fat);
        fclose(disk);
        return 5;
    }

    /* 4) Locate file in root directory (must pass an 11-byte string for the 8.3 name).
     *    For example: "KERNEL  BIN", "TEST    TXT", etc.
     */
    DirectoryEntry* fileEntry = findFile(argv[2]);
    if (!fileEntry) {
        fprintf(stderr, "Error: Could not find file '%s'\n", argv[2]);
        free(g_Fat);
        free(g_RootDirectory);
        fclose(disk);
        return 6;
    }

    /* 5) Allocate buffer for reading file contents */
    uint32_t fileSize = fileEntry->Size;
    uint8_t* buffer = (uint8_t*) malloc(fileSize + g_BootSector.BytesPerSector);
    if (!buffer) {
        fprintf(stderr, "Error: Could not allocate memory for file!\n");
        free(g_Fat);
        free(g_RootDirectory);
        fclose(disk);
        return 7;
    }
    memset(buffer, 0, fileSize + g_BootSector.BytesPerSector);

    /* 6) Read the file data from the disk image */
    if (!readFile(fileEntry, disk, buffer)) {
        fprintf(stderr, "Error: Could not read file '%s'\n", argv[2]);
        free(buffer);
        free(g_Fat);
        free(g_RootDirectory);
        fclose(disk);
        return 8;
    }

    /* 7) Print the file contents to stdout.
     *    We use 'isprint()' to check if the character is printable.
     *    Non-printable bytes are printed as <hex>.
     */
    for (size_t i = 0; i < fileSize; i++)
    {
        if (isprint(buffer[i]))
            fputc(buffer[i], stdout);
        else
            printf("<%02X>", buffer[i]);
    }
    printf("\n");

    /* Cleanup */
    free(buffer);
    free(g_Fat);
    free(g_RootDirectory);
    fclose(disk);

    return 0;
}
