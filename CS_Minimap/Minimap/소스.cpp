#include <iostream>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <Windows.h>
#include "SFmpqapi.h"
#include "SFmpq_static.h"
#include "SFmpqapi_no-lib.h"

using namespace std;
typedef unsigned long long QWORD;
MPQHANDLE* Save1[1024];
FILE* Save2[1024];
int Save1ptr = 0;
int Save2ptr = 0;

void f_ScloseAll(MPQHANDLE hMPQ)
{
    for (int i = 0; i < Save2ptr; i++)
        fclose(Save2[i]);
    for (int i = 0; i < Save1ptr; i++)
        SFileCloseFile(*Save1[i]);
    SFileCloseArchive(hMPQ); // MPQ 닫기
    Save1ptr = 0;
    Save2ptr = 0;
    return;
}

int f_Sopen(MPQHANDLE hMPQ, LPCSTR lpFileName, MPQHANDLE* hFile)
{
    // SFileOpenFileEx([MPQ핸들], [파일명], 0, &[리턴받을 파일핸들]);
    if (!SFileOpenFileEx(hMPQ, lpFileName, 0, hFile)) {
        printf("%s이(가) 존재하지 않습니다. [%d]\n", lpFileName, GetLastError());
        return -1;
    }
    Save1[Save1ptr++] = hFile;
}

int f_Scopy(MPQHANDLE hMPQ, MPQHANDLE* hFile, LPCSTR foutName, FILE** fout)
{
    fopen_s(fout, foutName, "w+b");
    if (*fout == NULL) {
        f_ScloseAll(hMPQ);
        printf("%s을 열 수 없습니다.\n", foutName);
        return -1;
    }
    Save2[Save2ptr++] = *fout;
    DWORD fsize = SFileGetFileSize(*hFile, NULL);
    char buffer[4096] = { 0 };
    while (fsize > 0) {
        DWORD transfersize = min(4096, fsize);
        DWORD readbyte;
        SFileReadFile(*hFile, buffer, transfersize, &readbyte, NULL); //fread에 해당
        if (readbyte != transfersize) {
            printf("SFileReadFile read %d bytes / %d bytes expected.\n", readbyte, transfersize);
            f_ScloseAll(hMPQ);
            return -1;
        }
        fwrite(buffer, 1, readbyte, *fout);
        fsize -= transfersize;
    }
}

int f_Fcopy(FILE** fin, FILE** fout, DWORD fsize)
{
    FILE* Fin = *fin;
    FILE* Fout = *fout;

    char buffer[4096] = { 0 };

    while (fsize > 0) {
        DWORD transfersize = min(4096, fsize);
        DWORD readbyte;
        readbyte = fread(buffer, transfersize, 1, Fin);
        if (readbyte != 1) {
            printf("ReadFile Failed.\n");
            return -1;
        }
        fwrite(buffer, transfersize, 1, Fout);
        fsize -= transfersize;
    }
}

void f_Swrite(MPQHANDLE hMPQ, LPCSTR finName, LPCSTR MPQName)
{
    MpqAddFileToArchive(hMPQ, finName, MPQName, MAFA_COMPRESS);
}

void f_SwriteWav(MPQHANDLE hMPQ, LPCSTR finName, LPCSTR MPQName)
{
    MpqAddWaveToArchive(hMPQ, finName, MPQName, MAFA_COMPRESS, MAWA_QUALITY_LOW);
}

int getTotalLine(FILE* fp) {
    int line = 0;
    char c;
    while ((c = fgetc(fp)) != EOF)
        if (c == '\n') line++;
    return(line);
}

DWORD UNIToff1 = 0;
DWORD UNIToff2 = 0;
DWORD UNIToff3 = 0;
DWORD Ret1 = 0;
DWORD Ret2 = 0;
int ModeX = 0;
int ModeY = 0;
void GetChkSection(FILE* fp, const char* Name)
{
    DWORD Key = Name[0] + Name[1] * 256 + Name[2] * 65536 + Name[3] * 16777216;
    int ret = 0, size;
    fseek(fp, 0, 2);
    size = ftell(fp);
    fseek(fp, 0, 0);

    DWORD Section[2];
    DWORD Check = 0;
    for (int i = 0; i < size;)
    {
        fseek(fp, i, 0);
        fread(Section, 4, 2, fp);
        if (Section[0] == Key)
        {
            Ret1 = i;
            Ret2 = i + (Section[1] + 8);
            Check = 1;
            break;
        }
        else
            i += (Section[1] + 8);
    }
    if (Check == 0)
        printf("맵의 %s단락을 찾을 수 없습니다.", Name);
    return;
}

DWORD NX;
DWORD MakeXY_4(DWORD A, DWORD B, DWORD C);
DWORD MakeXY_5(DWORD A, DWORD B, DWORD C);
DWORD MakeXY_9(DWORD A, DWORD B, DWORD C);
DWORD MakeXY_0(DWORD A, DWORD B, DWORD C);
DWORD MakeXY_14(DWORD A, DWORD B, DWORD C);
DWORD MakeXY_15(DWORD A, DWORD B, DWORD C);
DWORD MakeXY_19(DWORD A, DWORD B, DWORD C);
DWORD MakeXY_10(DWORD A, DWORD B, DWORD C);

void CreateUNITX1(FILE* fin,int j, int i,DWORD MY, BYTE PlayerID, BYTE P16)
{
    DWORD Buffer[9] =
    {
        0,0,0, 0x00030018, 0x64646400, 0, 0 ,0 ,0
    };
    BYTE* Bptr = (BYTE*)Buffer;
    if (PlayerID == 12)
    {
        Bptr[8] = 0xBC;
        Bptr[16] = P16;
        Buffer[1] = MakeXY_5(j, i, MY);
    }
    else
    {
        Bptr[8] = 0xD6;
        Bptr[16] = PlayerID;
        Buffer[1] = MakeXY_4(j, i, MY);
    }
    fwrite(Buffer, 0x24, 1, fin);
}
void CreateUNITX2(FILE* fin, int j, int i, DWORD MY, BYTE PlayerID, BYTE P16)
{
    DWORD Buffer[9] =
    {
        0,0,0, 0x00030018, 0x64646400, 0, 0 ,0 ,0
    };
    BYTE* Bptr = (BYTE*)Buffer;
    if (PlayerID == 12)
    {
        Bptr[8] = 0xBC;
        Bptr[16] = P16;
        Buffer[1] = MakeXY_0(j, i, MY);
    }
    else
    {
        Bptr[8] = 0xD6;
        Bptr[16] = PlayerID;
        Buffer[1] = MakeXY_9(j, i, MY);
    }
    fwrite(Buffer, 0x24, 1, fin);
}
void CreateUNITX3(FILE* fin, int j, int i, DWORD MY, BYTE PlayerID, BYTE P16)
{
    DWORD Buffer[9] =
    {
        0,0,0, 0x00030018, 0x64646400, 0, 0 ,0 ,0
    };
    BYTE* Bptr = (BYTE*)Buffer;
    if (PlayerID == 12)
    {
        Bptr[8] = 0xBC;
        Bptr[16] = P16;
        Buffer[1] = MakeXY_15(j, i, MY);
    }
    else
    {
        Bptr[8] = 0xD6;
        Bptr[16] = PlayerID;
        Buffer[1] = MakeXY_14(j, i, MY);
    }
    fwrite(Buffer, 0x24, 1, fin);
}
void CreateUNITX4(FILE* fin, int j, int i, DWORD MY, BYTE PlayerID, BYTE P16)
{
    DWORD Buffer[9] =
    {
        0,0,0, 0x00030018, 0x64646400, 0, 0 ,0 ,0
    };
    BYTE* Bptr = (BYTE*)Buffer;
    if (PlayerID == 12)
    {
        Bptr[8] = 0xBC;
        Bptr[16] = P16;
        Buffer[1] = MakeXY_10(j, i, MY);
    }
    else
    {
        Bptr[8] = 0xD6;
        Bptr[16] = PlayerID;
        Buffer[1] = MakeXY_19(j, i, MY);
    }
    fwrite(Buffer, 0x24, 1, fin);
}
void CreateUNITX(FILE* fin, DWORD XY, BYTE PlayerID, BYTE P16)
{
    DWORD Buffer[9] =
    {
        0,0,0, 0x00030018, 0x64646400, 0, 0 ,0 ,0
    };
    BYTE* Bptr = (BYTE*)Buffer;
    Buffer[1] = XY;
    if (PlayerID == 12)
    {
        Bptr[8] = 0xBC;
        Bptr[16] = P16;
    }
    else
    {
        Bptr[8] = 0xD6;
        Bptr[16] = PlayerID;
    }    
    fwrite(Buffer, 0x24, 1, fin);
}

void CreateUNIT(FILE* fin, DWORD XY, BYTE PlayerID, BYTE UnitID)
{
    DWORD Buffer[9] =
    {
        0,0,0, 0x00030018, 0x64646400, 0, 0 ,0 ,0
    };
    BYTE* Bptr = (BYTE*)Buffer;
    Buffer[1] = XY;
    if (UnitID == 0xD6 || UnitID == 0xBC)
        Bptr[8] = UnitID;
    else
        Bptr[8] = 0xAF + UnitID;
    Bptr[16] = PlayerID;
    fwrite(Buffer, 0x24, 1, fin);
}

DWORD MakeXY1(DWORD DIM, DWORD X, DWORD Y, DWORD MY)
{
    if (DIM == 96 || DIM == 128)
        return (32 + ModeX + X * 64) + (MY * 32 - (32 + ModeY + Y * 64)) * 0x10000;
    else if (DIM == 192 || DIM == 256)
        return (64 + ModeX + X * 128) + (MY * 32 - (64 + ModeY + Y * 128)) * 0x10000;
}

DWORD MakeXY2(DWORD Type,DWORD X, DWORD Y, DWORD MY)
{
    if (Type == 0)
        return (32 + ModeX + X * 64) + (MY * 32 - (16 + ModeY + Y * 64)) * 0x10000;
    else
        return (32 + ModeX + X * 64) + (MY * 32 - (48 + ModeY + Y * 64)) * 0x10000;
}

DWORD MakeXY3(DWORD X, DWORD Y, DWORD MY)
{
    return (64 + ModeX + X * 64) + (MY * 32 - (48 + ModeY + Y * 64)) * 0x10000;
}

DWORD MakeXY4(DWORD DIM, DWORD Type, DWORD X, DWORD Y, DWORD MY)
{
    if (Type == 0)
        return (64 + ModeX + X * 128) + (MY * 32 - (48 + ModeY + Y * 128)) * 0x10000;
    else
        return (64 + ModeX + X * 128) + (MY * 32 - (80 + ModeY + Y * 128)) * 0x10000;
}

DWORD MakeXY_1(DWORD X, DWORD Y, DWORD MY) // 2x2
{
    return (64 + X * 64) + (MY * 32 - (Y * 64)) * 0x10000;
}

DWORD MakeXY_2(DWORD X, DWORD Y, DWORD MY) // 4x2 Gas
{
    return (64 + X * 32) + (MY * 32 - (Y * 32)) * 0x10000;
}

DWORD MakeXY_3(DWORD X, DWORD Y, DWORD MY) // 4x3
{
    return (64 + X * 32) + (MY * 32 + 16 - (Y * 32)) * 0x10000;
}

DWORD MakeXY_4(DWORD X, DWORD Y, DWORD MY) // 4x4 
{
    return (64 + X * 16) + (MY * 32 + 32 - (Y * 16)) * 0x10000;
}

DWORD MakeXY_5(DWORD X, DWORD Y, DWORD MY) // 4x4 Gas
{
    return (64 + X * 16) + (MY * 32 + 16 - (Y * 16)) * 0x10000;
}

DWORD MakeXY_6(DWORD X, DWORD Y, DWORD MY) // 2x2
{
    return (X * 64) + (MY * 32 - (Y * 64)) * 0x10000;
}

DWORD MakeXY_7(DWORD X, DWORD Y, DWORD MY) // 4x2 Gas
{
    return (-32 + X * 32) + (MY * 32 - (Y * 32)) * 0x10000;
}

DWORD MakeXY_8(DWORD X, DWORD Y, DWORD MY) // 4x3
{
    return (-32 + X * 32) + (MY * 32 + 16 - (Y * 32)) * 0x10000;
}

DWORD MakeXY_9(DWORD X, DWORD Y, DWORD MY) // 4x4 
{
    return (16 + X * 16) + (MY * 32 + 32 - (Y * 16)) * 0x10000;
}

DWORD MakeXY_0(DWORD X, DWORD Y, DWORD MY) // 4x4 Gas
{
    return (16 + X * 16) + (MY * 32 + 16 - (Y * 16)) * 0x10000;
}

DWORD MakeXY_11(DWORD X, DWORD Y, DWORD MY) // 2x2
{
    return (64 + X * 64) + (MY * 32 - (64 + Y * 64)) * 0x10000;
}

DWORD MakeXY_12(DWORD X, DWORD Y, DWORD MY) // 4x2 Gas
{
    return (64 + X * 32) + (MY * 32 - (0 + Y * 32)) * 0x10000;
}

DWORD MakeXY_13(DWORD X, DWORD Y, DWORD MY) // 4x3
{
    return (64 + X * 32) + (MY * 32 + 16 - (32 + Y * 32)) * 0x10000;
}

DWORD MakeXY_14(DWORD X, DWORD Y, DWORD MY) // 4x4 
{
    return (64 + X * 16) + (MY * 32 - (16 + Y * 16)) * 0x10000;
}

DWORD MakeXY_15(DWORD X, DWORD Y, DWORD MY) // 4x4 Gas
{
    return (64 + X * 16) + (MY * 32 - (32 + Y * 16)) * 0x10000;
}

DWORD MakeXY_16(DWORD X, DWORD Y, DWORD MY) // 2x2
{
    return (X * 64) + (MY * 32 - (64 + Y * 64)) * 0x10000;
}

DWORD MakeXY_17(DWORD X, DWORD Y, DWORD MY) // 4x2 Gas
{
    return (-32 + X * 32) + (MY * 32 - (0 + Y * 32)) * 0x10000;
}

DWORD MakeXY_18(DWORD X, DWORD Y, DWORD MY) // 4x3
{
    return (-32 + X * 32) + (MY * 32 + 16 - (32 + Y * 32)) * 0x10000;
}

DWORD MakeXY_19(DWORD X, DWORD Y, DWORD MY) // 4x4 
{
    return (16 + X * 16) + (MY * 32 - (16 + Y * 16)) * 0x10000;
}

DWORD MakeXY_10(DWORD X, DWORD Y, DWORD MY) // 4x4 Gas
{
    return (16 + X * 16) + (MY * 32 - (32 + Y * 16)) * 0x10000;
}

void LCreateUNIT1(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal  
    {
        if (DIM == 64)
        {
            CreateUNIT(fin, MakeXY_4(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
        else if (DIM == 96 || DIM == 128)
        {
            CreateUNIT(fin, MakeXY_3(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
        else if (DIM == 192 || DIM == 256)
        {
            CreateUNIT(fin, MakeXY_1(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
    }
    else if (BTemp == 12)
    {
        if (DIM == 64)
        {
            CreateUNIT(fin, MakeXY_5(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
        else if (DIM == 96 || DIM == 128)
        {
            CreateUNIT(fin, MakeXY_2(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
        else if (DIM == 192 || DIM == 256)
        {
            CreateUNIT(fin, MakeXY_1(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
    }
}

void LCreateUNIT2(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal
    {
        if (DIM == 64)
        {
            CreateUNIT(fin, MakeXY_9(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
        else if (DIM == 96 || DIM == 128)
        {
            CreateUNIT(fin, MakeXY_8(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
        else if (DIM == 192 || DIM == 256)
        {
            CreateUNIT(fin, MakeXY_6(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
    }
    else if (BTemp == 12)
    {
        if (DIM == 64)
        {
            CreateUNIT(fin, MakeXY_0(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
        else if (DIM == 96 || DIM == 128)
        {
            CreateUNIT(fin, MakeXY_7(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
        else if (DIM == 192 || DIM == 256)
        {
            CreateUNIT(fin, MakeXY_6(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
    }
}

void LCreateUNIT3(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal
    {
        if (DIM == 64)
        {
            CreateUNIT(fin, MakeXY_14(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
        else if (DIM == 96 || DIM == 128)
        {
            CreateUNIT(fin, MakeXY_13(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
        else if (DIM == 192 || DIM == 256)
        {
            CreateUNIT(fin, MakeXY_11(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
    }
    else if (BTemp == 12)
    {
        if (DIM == 64)
        {
            CreateUNIT(fin, MakeXY_15(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
        else if (DIM == 96 || DIM == 128)
        {
            CreateUNIT(fin, MakeXY_12(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
        else if (DIM == 192 || DIM == 256)
        {
            CreateUNIT(fin, MakeXY_11(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
    }
}

void LCreateUNIT4(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal
    {
        if (DIM == 64)
        {
            CreateUNIT(fin, MakeXY_19(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
        else if (DIM == 96 || DIM == 128)
        {
            CreateUNIT(fin, MakeXY_18(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
        else if (DIM == 192 || DIM == 256)
        {
            CreateUNIT(fin, MakeXY_16(j, i, MY), BTemp, 0xD6);
            (*CCount)++;
        }
    }
    else if (BTemp == 12)
    {
        if (DIM == 64)
        {
            CreateUNIT(fin, MakeXY_10(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
        else if (DIM == 96 || DIM == 128)
        {
            CreateUNIT(fin, MakeXY_17(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
        else if (DIM == 192 || DIM == 256)
        {
            CreateUNIT(fin, MakeXY_16(j, i, MY), Player, 0xBC);
            (*CCount)++;
        }
    }
}


DWORD ReadBit(BYTE* ptr, DWORD Bit)
{
    DWORD Mask = 1 << Bit;
    DWORD Ret = (*ptr & Mask) >> Bit;
    return Ret;
}

void WriteBit(BYTE* ptr, DWORD Bit, DWORD Value)
{
    if (Value == 0)
    {
        DWORD Mask = 1 << Bit;
        *ptr = *ptr & (0xFF - Mask);
    }
    else 
    {
        DWORD Mask = 1 << Bit;
        *ptr = *ptr | (0 + Mask);
    }
}

int Check1(BYTE* IPtr, int LX, int Index,int Type)
{
    if (Type == 0)
    {
        int list[4] = { Index + 1, Index + 2,Index + 2 + LX, Index + 3 + LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[3] = { Index + 1, Index + 2,Index + 2 + LX};
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index + 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[1] = { Index + 1};
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index + 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check2(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index + 1, Index + 2,Index + 2 + LX, Index + 3 + LX, Index + 3, Index + 4 + LX , Index + 4,Index + 5 + LX };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index + 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index + 1, Index + 2,Index + 2 + LX, Index + 3 + LX, Index + 3, Index + 4 + LX , Index + 4 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index + 1, Index + 2,Index + 2 + LX, Index + 3 + LX, Index + 3 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[3] = { Index + 1, Index + 2,Index + 2 + LX};
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index + 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[1] = { Index + 1 };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index + 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check3(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index + 2,Index + 2 + LX, Index + 3 + LX, Index + 3, Index + 4 + LX , Index + 4,Index + 5 + LX ,Index + 6 + LX };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index + 2 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index + 2,Index + 2 + LX, Index + 3 + LX, Index + 3, Index + 4 + LX , Index + 4,Index + 5 + LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + 2 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[6] = { Index + 2,Index + 2 + LX, Index + 3 + LX, Index + 3, Index + 4 + LX , Index + 4};
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + 2 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index + 2,Index + 2 + LX, Index + 3 + LX, Index + 3 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 2 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index + 2,Index + 2 + LX };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index + 2 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check4(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[7] = { Index + 3 + LX, Index + 3, Index + 4 + LX , Index + 4,Index + 5 + LX ,Index + 6 + LX ,Index + 7 + LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + 3 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[6] = { Index + 3 + LX, Index + 3, Index + 4 + LX , Index + 4,Index + 5 + LX ,Index + 6 + LX };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + 3 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = {  Index + 3 + LX, Index + 3, Index + 4 + LX , Index + 4,Index + 5 + LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + 3 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index + 3 + LX, Index + 3, Index + 4 + LX , Index + 4 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 3 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index + 3 + LX, Index + 3 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index + 3 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check5(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[4] = { Index - LX, Index - 2*LX,Index - 2*LX - 1, Index - 3*LX - 1 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[3] = { Index - LX, Index - 2 * LX,Index - 2 * LX - 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index - 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[1] = { Index - LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index - 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check6(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index - LX, Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1, Index - 4 * LX, Index - 5 * LX - 1 };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index - LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index - LX, Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1, Index - 4 * LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index - LX, Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[3] = { Index - LX, Index - 2 * LX,Index - 2 * LX - 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index - LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[1] = { Index - LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index - LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check7(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1, Index - 4 * LX, Index - 5 * LX - 1 , Index - 6 * LX - 1 };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index - 2*LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1, Index - 4 * LX, Index - 5 * LX - 1 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - 2*LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[6] = { Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1, Index - 4 * LX };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - 2*LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 2*LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index - 2 * LX,Index - 2 * LX - 1 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index - 2*LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check8(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[7] = { Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1, Index - 4 * LX, Index - 5 * LX - 1 , Index - 6 * LX - 1, Index - 7 * LX - 1 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - 3 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[6] = { Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1, Index - 4 * LX, Index - 5 * LX - 1 , Index - 6 * LX - 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - 3 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1, Index - 4 * LX, Index - 5 * LX - 1 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - 3 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1, Index - 4 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 3 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index - 3 * LX - 1, Index - 3 * LX };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index - 3 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check9(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[6] = { Index - LX, Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[5] = { Index - LX, Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[3] = { Index - LX, Index - 2 * LX,Index - 2 * LX - 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index - LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[1] = { Index - LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index - LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check0(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[6] = { Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1, Index - 5 * LX - 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[5] = { Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX, Index - 4 * LX - 1 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[4] = { Index - 2 * LX,Index - 2 * LX - 1, Index - 3 * LX - 1, Index - 3 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[2] = { Index - 2 * LX,Index - 2 * LX - 1 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index - 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check11(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[4] = { Index - 1, Index - 2,Index - 2 + LX, Index - 3 + LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[3] = { Index - 1, Index - 2,Index - 2 + LX };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index - 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[1] = { Index - 1 };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index - 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check12(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index - 1, Index - 2,Index - 2 + LX, Index - 3 + LX, Index - 3, Index - 4 + LX , Index - 4,Index - 5 + LX };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index - 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index - 1, Index - 2,Index - 2 + LX, Index - 3 + LX, Index - 3, Index - 4 + LX , Index - 4 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index - 1, Index - 2,Index - 2 + LX, Index - 3 + LX, Index - 3 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[3] = { Index - 1, Index - 2,Index - 2 + LX };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index - 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[1] = { Index - 1 };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index - 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check13(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index - 2,Index - 2 + LX, Index - 3 + LX, Index - 3, Index - 4 + LX , Index - 4,Index - 5 + LX ,Index - 6 + LX };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index - 2 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index - 2,Index - 2 + LX, Index - 3 + LX, Index - 3, Index - 4 + LX , Index - 4,Index - 5 + LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - 2 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[6] = { Index - 2,Index - 2 + LX, Index - 3 + LX, Index - 3, Index - 4 + LX , Index - 4 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - 2 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index - 2,Index - 2 + LX, Index - 3 + LX, Index - 3 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 2 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index - 2,Index - 2 + LX };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index - 2 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check14(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[7] = { Index - 3 + LX, Index - 3, Index - 4 + LX , Index - 4,Index - 5 + LX ,Index - 6 + LX ,Index - 7 + LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - 3 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[6] = { Index - 3 + LX, Index - 3, Index - 4 + LX , Index - 4,Index - 5 + LX ,Index - 6 + LX };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - 3 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index - 3 + LX, Index - 3, Index - 4 + LX , Index - 4,Index - 5 + LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - 3 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index - 3 + LX, Index - 3, Index - 4 + LX , Index - 4 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 3 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index - 3 + LX, Index - 3 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index - 3 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check15(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[4] = { Index - LX, Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[3] = { Index - LX, Index - 2 * LX,Index - 2 * LX + 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index + 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[1] = { Index - LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index + 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check16(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index - LX, Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1, Index - 4 * LX, Index - 5 * LX + 1 };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index - LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index - LX, Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1, Index - 4 * LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index - LX, Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[3] = { Index - LX, Index - 2 * LX,Index - 2 * LX + 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index - LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[1] = { Index - LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index - LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check17(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1, Index - 4 * LX, Index - 5 * LX + 1 , Index - 6 * LX + 1 };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index - 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1, Index - 4 * LX, Index - 5 * LX + 1 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[6] = { Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1, Index - 4 * LX };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index - 2 * LX,Index - 2 * LX + 1 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index - 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check18(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[7] = { Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1, Index - 4 * LX, Index - 5 * LX + 1 , Index - 6 * LX + 1, Index - 7 * LX + 1 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - 3 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[6] = { Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1, Index - 4 * LX, Index - 5 * LX + 1 , Index - 6 * LX + 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - 3 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1, Index - 4 * LX, Index - 5 * LX + 1 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - 3 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1, Index - 4 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 3 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index - 3 * LX + 1, Index - 3 * LX };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index - 3 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check19(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[6] = { Index - LX, Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[5] = { Index - LX, Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[3] = { Index - LX, Index - 2 * LX,Index - 2 * LX + 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index - LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[1] = { Index - LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index - LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check10(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[6] = { Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1, Index - 5 * LX + 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[5] = { Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX, Index - 4 * LX + 1 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[4] = { Index - 2 * LX,Index - 2 * LX + 1, Index - 3 * LX + 1, Index - 3 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[2] = { Index - 2 * LX,Index - 2 * LX + 1 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index - 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check21(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[4] = { Index + 1, Index + 2,Index + 2 - LX, Index + 3 - LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[3] = { Index + 1, Index + 2,Index + 2 - LX };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index + 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[1] = { Index + 1 };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index + 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check22(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index + 1, Index + 2,Index + 2 - LX, Index + 3 - LX, Index + 3, Index + 4 - LX , Index + 4,Index + 5 - LX };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index + 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index + 1, Index + 2,Index + 2 - LX, Index + 3 - LX, Index + 3, Index + 4 - LX , Index + 4 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index + 1, Index + 2,Index + 2 - LX, Index + 3 - LX, Index + 3 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[3] = { Index + 1, Index + 2,Index + 2 - LX };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index + 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[1] = { Index + 1 };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index + 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check23(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index + 2,Index + 2 - LX, Index + 3 - LX, Index + 3, Index + 4 - LX , Index + 4,Index + 5 - LX ,Index + 6 - LX };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index + 2 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index + 2,Index + 2 - LX, Index + 3 - LX, Index + 3, Index + 4 - LX , Index + 4,Index + 5 - LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + 2 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[6] = { Index + 2,Index + 2 - LX, Index + 3 - LX, Index + 3, Index + 4 - LX , Index + 4 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + 2 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index + 2,Index + 2 - LX, Index + 3 - LX, Index + 3 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 2 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index + 2,Index + 2 - LX };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index + 2 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check24(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[7] = { Index + 3 - LX, Index + 3, Index + 4 - LX , Index + 4,Index + 5 - LX ,Index + 6 - LX ,Index + 7 - LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + 3 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[6] = { Index + 3 - LX, Index + 3, Index + 4 - LX , Index + 4,Index + 5 - LX ,Index + 6 - LX };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + 3 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index + 3 - LX, Index + 3, Index + 4 - LX , Index + 4,Index + 5 - LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + 3 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index + 3 - LX, Index + 3, Index + 4 - LX , Index + 4 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 3 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index + 3 - LX, Index + 3 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index + 3 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check25(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[4] = { Index + LX, Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[3] = { Index + LX, Index + 2 * LX,Index + 2 * LX - 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index - 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[1] = { Index + LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index - 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check26(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index + LX, Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1, Index + 4 * LX, Index + 5 * LX - 1 };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index + LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index + LX, Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1, Index + 4 * LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index + LX, Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[3] = { Index + LX, Index + 2 * LX,Index + 2 * LX - 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index + LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[1] = { Index + LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index + LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check27(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1, Index + 4 * LX, Index + 5 * LX - 1 , Index + 6 * LX - 1 };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index + 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1, Index + 4 * LX, Index + 5 * LX - 1 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[6] = { Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1, Index + 4 * LX };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index + 2 * LX,Index + 2 * LX - 1 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index + 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check28(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[7] = { Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1, Index + 4 * LX, Index + 5 * LX - 1 , Index + 6 * LX - 1, Index + 7 * LX - 1 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + 3 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[6] = { Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1, Index + 4 * LX, Index + 5 * LX - 1 , Index + 6 * LX - 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + 3 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1, Index + 4 * LX, Index + 5 * LX - 1 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + 3 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1, Index + 4 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 3 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index + 3 * LX - 1, Index + 3 * LX };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index + 3 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check29(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[6] = { Index + LX, Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[5] = { Index + LX, Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[3] = { Index + LX, Index + 2 * LX,Index + 2 * LX - 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index + LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[1] = { Index + LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index + LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check20(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[6] = { Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1, Index + 5 * LX - 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[5] = { Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX, Index + 4 * LX - 1 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[4] = { Index + 2 * LX,Index + 2 * LX - 1, Index + 3 * LX - 1, Index + 3 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[2] = { Index + 2 * LX,Index + 2 * LX - 1 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index + 2 * LX - 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check31(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[4] = { Index - 1, Index - 2,Index - 2 - LX, Index - 3 - LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[3] = { Index - 1, Index - 2,Index - 2 - LX };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index - 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[1] = { Index - 1 };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index - 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check32(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index - 1, Index - 2,Index - 2 - LX, Index - 3 - LX, Index - 3, Index - 4 - LX , Index - 4,Index - 5 - LX };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index - 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index - 1, Index - 2,Index - 2 - LX, Index - 3 - LX, Index - 3, Index - 4 - LX , Index - 4 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index - 1, Index - 2,Index - 2 - LX, Index - 3 - LX, Index - 3 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[3] = { Index - 1, Index - 2,Index - 2 - LX };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index - 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[1] = { Index - 1 };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index - 1 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check33(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index - 2,Index - 2 - LX, Index - 3 - LX, Index - 3, Index - 4 - LX , Index - 4,Index - 5 - LX ,Index - 6 - LX };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index - 2 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index - 2,Index - 2 - LX, Index - 3 - LX, Index - 3, Index - 4 - LX , Index - 4,Index - 5 - LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - 2 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[6] = { Index - 2,Index - 2 - LX, Index - 3 - LX, Index - 3, Index - 4 - LX , Index - 4 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - 2 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index - 2,Index - 2 - LX, Index - 3 - LX, Index - 3 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 2 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index - 2,Index - 2 - LX };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index - 2 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check34(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[7] = { Index - 3 - LX, Index - 3, Index - 4 - LX , Index - 4,Index - 5 - LX ,Index -6 - LX ,Index - 7 - LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index - 3 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[6] = { Index - 3 - LX, Index - 3, Index - 4 - LX , Index - 4,Index - 5 - LX ,Index - 6 - LX };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index - 3 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index - 3 - LX, Index - 3, Index - 4 - LX , Index - 4,Index - 5 - LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index - 3 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index - 3 - LX, Index - 3, Index - 4 - LX , Index - 4 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index - 3 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index - 3 - LX, Index - 3 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index - 3 - LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check35(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[4] = { Index + LX, Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1 };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[3] = { Index + LX, Index + 2 * LX,Index + 2 * LX + 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index + 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[1] = { Index + LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index + 1 + LX] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check36(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index + LX, Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1, Index + 4 * LX, Index + 5 * LX + 1 };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index + LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index + LX, Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1, Index + 4 * LX };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index + LX, Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[3] = { Index + LX, Index + 2 * LX,Index + 2 * LX + 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index + LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[1] = { Index + LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index + LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check37(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[8] = { Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1, Index + 4 * LX, Index + 5 * LX + 1 , Index + 6 * LX + 1 };
        for (int k = 0; k < 8; k++)
        {
            if (IPtr[Index + 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[7] = { Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1, Index + 4 * LX, Index + 5 * LX + 1 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[6] = { Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1, Index + 4 * LX };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index + 2 * LX,Index + 2 * LX + 1 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index + 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check38(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[7] = { Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1, Index + 4 * LX, Index + 5 * LX + 1 , Index + 6 * LX + 1, Index + 7 * LX + 1 };
        for (int k = 0; k < 7; k++)
        {
            if (IPtr[Index + 3 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[6] = { Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1, Index + 4 * LX, Index + 5 * LX + 1 , Index + 6 * LX + 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + 3 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[5] = { Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1, Index + 4 * LX, Index + 5 * LX + 1 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + 3 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[4] = { Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1, Index + 4 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 3 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 4)
    {
        int list[2] = { Index + 3 * LX + 1, Index + 3 * LX };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index + 3 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check39(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[6] = { Index + LX, Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[5] = { Index + LX, Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[3] = { Index + LX, Index + 2 * LX,Index + 2 * LX + 1 };
        for (int k = 0; k < 3; k++)
        {
            if (IPtr[Index + LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[1] = { Index + LX };
        for (int k = 0; k < 1; k++)
        {
            if (IPtr[Index + LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

int Check30(BYTE* IPtr, int LX, int Index, int Type)
{
    if (Type == 0)
    {
        int list[6] = { Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1, Index + 5 * LX + 1 };
        for (int k = 0; k < 6; k++)
        {
            if (IPtr[Index + 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 1)
    {
        int list[5] = { Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX, Index + 4 * LX + 1 };
        for (int k = 0; k < 5; k++)
        {
            if (IPtr[Index + 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 2)
    {
        int list[4] = { Index + 2 * LX,Index + 2 * LX + 1, Index + 3 * LX + 1, Index + 3 * LX };
        for (int k = 0; k < 4; k++)
        {
            if (IPtr[Index + 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else if (Type == 3)
    {
        int list[2] = { Index + 2 * LX,Index + 2 * LX + 1 };
        for (int k = 0; k < 2; k++)
        {
            if (IPtr[Index + 2 * LX + 1] != IPtr[list[k]])
            {
                return 0;
            }
        }
    }
    else
        return 0;
    return 1;
}

BYTE LCreateUNIT1A(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 14 : Height / Width Black
    {
        CreateUNIT(fin, MakeXY_1(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_1(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j <= 1 || j == LX - 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i >= LY - 1 || IPtr[index - 1] == IPtr[index + NX] || IPtr[index + 1] == IPtr[index + NX])
            {
                Color = 0;
                while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                    Color++;
            }
            else
                Color = IPtr[index + NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_1(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_1(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 14)
    {
        if (i >= LY-2 || i == 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_1(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (j <= 1 || i <= 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j == LX - 2 && i == 3)
            {
                if (i >= LY - 1 || IPtr[index - 2] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (j == LX - 2)
            {
                if (i >= LY - 1 || IPtr[index - 2] == IPtr[index + NX] || IPtr[index - 1 - 3 * NX] == IPtr[index + NX] || IPtr[index - 2 - 3 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2] || Color == IPtr[index - 1 - 3 * NX] || Color == IPtr[index - 2 - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (i == 3)
            {
                if (i >= LY - 1 || IPtr[index - 2] == IPtr[index + NX] || IPtr[index + 1] == IPtr[index + NX] || IPtr[index + 1 - NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2] || Color == IPtr[index + 1] || Color == IPtr[index + 1 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else 
            {
                if (i >= LY - 1 || IPtr[index - 2] == IPtr[index + NX] || IPtr[index + 1] == IPtr[index + NX] || IPtr[index + 1 - NX] == IPtr[index + NX] || IPtr[index - 1 - 3 * NX] == IPtr[index + NX] || IPtr[index - 2 - 3 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2] || Color == IPtr[index + 1] || Color == IPtr[index + 1 - NX] || Color == IPtr[index - 1 - 3*NX] || Color == IPtr[index - 2 - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            if (Color == 12)
                CreateUNIT(fin, MakeXY_1(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_1(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - NX] < 13)
                IPtr[index - NX] = (BYTE)Color;
            if (IPtr[index - 1 - 2 * NX] < 13)
                IPtr[index - 1 - 2 * NX] = (BYTE)Color;
            if (IPtr[index - 2 - 2 * NX] < 13)
                IPtr[index - 2 - 2 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT1B(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_1(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_1(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 14)
    {
        if (i >= LY-2 || i == 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j <= 0 || IPtr[index + NX] == IPtr[index - 1] || IPtr[index - NX] == IPtr[index - 1])
            {
                Color = 0;
                while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                    Color++;
            }
            else
                Color = IPtr[index - 1];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_1(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_1(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 13)
    {
        if (j <= 1 || j == LX-2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                Color++;
            CreateUNIT(fin, MakeXY_1(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (i >= LY-2 || j >= LX-3)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i == 1 && j == LX-4)
            {
                if (j <= 0 || IPtr[index + 2*NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (i == 1)
            {
                if (j <= 0 || IPtr[index + 2 * NX] == IPtr[index - 1] || IPtr[index + NX + 3] == IPtr[index - 1] || IPtr[index + 2*NX + 3] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX] || Color == IPtr[index + NX + 3] || Color == IPtr[index + 2*NX + 3])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (j == LX - 4)
            {
                if (j <= 0 || IPtr[index + 2 * NX] == IPtr[index - 1] || IPtr[index - NX] == IPtr[index - 1] || IPtr[index - NX + 1] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX] || Color == IPtr[index - NX] || Color == IPtr[index - NX + 1])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else
            {
                if (j <= 0 || IPtr[index + 2 * NX] == IPtr[index - 1] || IPtr[index - NX] == IPtr[index - 1] || IPtr[index - NX + 1] == IPtr[index - 1] || IPtr[index + NX + 3] == IPtr[index - 1] || IPtr[index + 2 * NX + 3] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX] || Color == IPtr[index - NX] || Color == IPtr[index - NX + 1] || Color == IPtr[index + NX + 3] || Color == IPtr[index + 2 * NX + 3])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_1(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_1(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 1] < 13)
                IPtr[index + 1] = (BYTE)Color;
            if (IPtr[index + NX + 2] < 13)
                IPtr[index + NX + 2] = (BYTE)Color;
            if (IPtr[index +2*NX + 2] < 13)
                IPtr[index +2*NX + 2] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT2A(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal
    {
        CreateUNIT(fin, MakeXY_6(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_6(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j >= LX-2 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i >= LY - 1 || IPtr[index + 1] == IPtr[index + NX] || IPtr[index - 1] == IPtr[index + NX])
            {
                Color = 0;
                while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                    Color++;
            }
            else
                Color = IPtr[index + NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_6(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_6(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 14)
    {
        if (i >= LY - 2 || i == 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_6(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (j >= LX-2 || i <= 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j == TX && i == 3)
            {
                if (i >= LY - 1 || IPtr[index + 2] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (j == TX)
            {
                if (i >= LY - 1 || IPtr[index + 2] == IPtr[index + NX] || IPtr[index + 1 - 3 * NX] == IPtr[index + NX] || IPtr[index + 2 - 3 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2] || Color == IPtr[index + 1 - 3 * NX] || Color == IPtr[index + 2 - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (i == 3)
            {
                if (i >= LY - 1 || IPtr[index + 2] == IPtr[index + NX] || IPtr[index - 1] == IPtr[index + NX] || IPtr[index - 1 - NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2] || Color == IPtr[index - 1] || Color == IPtr[index - 1 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else
            {
                if (i >= LY - 1 || IPtr[index + 2] == IPtr[index + NX] || IPtr[index - 1] == IPtr[index + NX] || IPtr[index - 1 - NX] == IPtr[index + NX] || IPtr[index + 1 - 3 * NX] == IPtr[index + NX] || IPtr[index + 2 - 3 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2] || Color == IPtr[index - 1] || Color == IPtr[index - 1 - NX] || Color == IPtr[index + 1 - 3 * NX] || Color == IPtr[index + 2 - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_6(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_6(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - NX] < 13)
                IPtr[index - NX] = (BYTE)Color;
            if (IPtr[index + 1 - 2 * NX] < 13)
                IPtr[index + 1 - 2 * NX] = (BYTE)Color;
            if (IPtr[index + 2 - 2 * NX] < 13)
                IPtr[index + 2 - 2 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT2B(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_6(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_6(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 14)
    {
        if (i >= LY - 2 || i == 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j >= LX-1 || IPtr[index + NX] == IPtr[index + 1] || IPtr[index - NX] == IPtr[index + 1])
            {
                Color = 0;
                while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                    Color++;
            }
            else
                Color = IPtr[index + 1];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_6(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_6(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 13)
    {
        if (j >= LX-2 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                Color++;
            CreateUNIT(fin, MakeXY_6(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (i >= LY - 2 || j <= TX - 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i == 1 && j == TX - 2)
            {
                if (j >= LX-1 || IPtr[index + 2 * NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (i == 1)
            {
                if (j >= LX - 1 || IPtr[index + 2 * NX] == IPtr[index + 1] || IPtr[index + NX - 3] == IPtr[index + 1] || IPtr[index + 2 * NX - 3] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX] || Color == IPtr[index + NX - 3] || Color == IPtr[index + 2 * NX - 3])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (j == TX - 2)
            {
                if (j >= LX - 1 || IPtr[index + 2 * NX] == IPtr[index + 1] || IPtr[index - NX] == IPtr[index + 1] || IPtr[index - NX - 1] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX] || Color == IPtr[index - NX] || Color == IPtr[index - NX - 1])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else
            {
                if (j >= LX - 1 || IPtr[index + 2 * NX] == IPtr[index + 1] || IPtr[index - NX] == IPtr[index + 1] || IPtr[index - NX - 1] == IPtr[index + 1] || IPtr[index + NX - 3] == IPtr[index + 1] || IPtr[index + 2 * NX - 3] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX] || Color == IPtr[index - NX] || Color == IPtr[index - NX - 1] || Color == IPtr[index + NX - 3] || Color == IPtr[index + 2 * NX - 3])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_6(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_6(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - 1] < 13)
                IPtr[index - 1] = (BYTE)Color;
            if (IPtr[index + NX - 2] < 13)
                IPtr[index + NX - 2] = (BYTE)Color;
            if (IPtr[index + 2 * NX - 2] < 13)
                IPtr[index + 2 * NX - 2] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT3A(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal
    {
        CreateUNIT(fin, MakeXY_11(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_11(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j <= 1 || j == LX - 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i <= 0 || IPtr[index - 1] == IPtr[index - NX] || IPtr[index + 1] == IPtr[index - NX])
            {
                Color = 0;
                while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                    Color++;
            }
            else
                Color = IPtr[index - NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_11(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_11(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 14)
    {
        if (i <= 1 || i == LY-TY-1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_11(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (j <= 1 || i >= LY-TY-2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j == LX - 2 && i == LY-TY-3)
            {
                if (i <= 0 || IPtr[index - 2] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (j == LX - 2)
            {
                if (i <= 0 || IPtr[index - 2] == IPtr[index - NX] || IPtr[index - 1 + 3 * NX] == IPtr[index - NX] || IPtr[index - 2 + 3 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2] || Color == IPtr[index - 1 + 3 * NX] || Color == IPtr[index - 2 + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (i == LY - TY - 3)
            {
                if (i <= 0 || IPtr[index - 2] == IPtr[index - NX] || IPtr[index + 1] == IPtr[index - NX] || IPtr[index + 1 + NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2] || Color == IPtr[index + 1] || Color == IPtr[index + 1 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else
            {
                if (i <= 0 || IPtr[index - 2] == IPtr[index - NX] || IPtr[index + 1] == IPtr[index - NX] || IPtr[index + 1 + NX] == IPtr[index - NX] || IPtr[index - 1 + 3 * NX] == IPtr[index - NX] || IPtr[index - 2 + 3 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2] || Color == IPtr[index + 1] || Color == IPtr[index + 1 + NX] || Color == IPtr[index - 1 + 3 * NX] || Color == IPtr[index - 2 + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_11(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_11(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + NX] < 13)
                IPtr[index + NX] = (BYTE)Color;
            if (IPtr[index - 1 + 2 * NX] < 13)
                IPtr[index - 1 + 2 * NX] = (BYTE)Color;
            if (IPtr[index - 2 + 2 * NX] < 13)
                IPtr[index - 2 + 2 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT3B(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_11(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_11(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 14)
    {
        if (i <= 1 || i == LY-TY-1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j <= 0 || IPtr[index - NX] == IPtr[index - 1] || IPtr[index + NX] == IPtr[index - 1])
            {
                Color = 0;
                while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                    Color++;
            }
            else
                Color = IPtr[index - 1];
            CreateUNIT(fin, MakeXY_11(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 13)
    {
        if (j <= 1 || j == LX - 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                Color++;
            if (Color == 12)
                CreateUNIT(fin, MakeXY_11(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_11(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (i <= 1 || j >= LX - 3)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i == LY-TY-1 && j == LX - 4)
            {
                if (j <= 0 || IPtr[index - 2 * NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (i == LY - TY-1)
            {
                if (j <= 0 || IPtr[index - 2 * NX] == IPtr[index - 1] || IPtr[index - NX + 3] == IPtr[index - 1] || IPtr[index - 2 * NX + 3] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX] || Color == IPtr[index - NX + 3] || Color == IPtr[index - 2 * NX + 3])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (j == LX - 4)
            {
                if (j <= 0 || IPtr[index - 2 * NX] == IPtr[index - 1] || IPtr[index + NX] == IPtr[index - 1] || IPtr[index + NX + 1] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX] || Color == IPtr[index + NX] || Color == IPtr[index + NX + 1])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else
            {
                if (j <= 0 || IPtr[index - 2 * NX] == IPtr[index - 1] || IPtr[index + NX] == IPtr[index - 1] || IPtr[index + NX + 1] == IPtr[index - 1] || IPtr[index - NX + 3] == IPtr[index - 1] || IPtr[index - 2 * NX + 3] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX] || Color == IPtr[index + NX] || Color == IPtr[index + NX + 1] || Color == IPtr[index - NX + 3] || Color == IPtr[index - 2 * NX + 3])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_11(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_11(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 1] < 13)
                IPtr[index + 1] = (BYTE)Color;
            if (IPtr[index - NX + 2] < 13)
                IPtr[index - NX + 2] = (BYTE)Color;
            if (IPtr[index - 2 * NX + 2] < 13)
                IPtr[index - 2 * NX + 2] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT4A(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal
    {
        CreateUNIT(fin, MakeXY_16(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_16(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j >= LX-2 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i <= 0 || IPtr[index + 1] == IPtr[index - NX] || IPtr[index - 1] == IPtr[index - NX])
            {
                Color = 0;
                while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                    Color++;
            }
            else
                Color = IPtr[index - NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_16(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_16(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 14)
    {
        if (i <= 1 || i == LY - TY-1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_16(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (j >= LX-2 || i >= LY - TY - 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j == TX && i == LY - TY - 3)
            {
                if (i <= 0 || IPtr[index + 2] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (j == TX)
            {
                if (i <= 0 || IPtr[index + 2] == IPtr[index - NX] || IPtr[index + 1 + 3 * NX] == IPtr[index - NX] || IPtr[index + 2 + 3 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2] || Color == IPtr[index + 1 + 3 * NX] || Color == IPtr[index + 2 + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (i == LY - TY - 3)
            {
                if (i <= 0 || IPtr[index + 2] == IPtr[index - NX] || IPtr[index - 1] == IPtr[index - NX] || IPtr[index - 1 + NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2] || Color == IPtr[index - 1] || Color == IPtr[index - 1 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else
            {
                if (i <= 0 || IPtr[index + 2] == IPtr[index - NX] || IPtr[index - 1] == IPtr[index - NX] || IPtr[index - 1 + NX] == IPtr[index - NX] || IPtr[index + 1 + 3 * NX] == IPtr[index - NX] || IPtr[index + 2 + 3 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2] || Color == IPtr[index - 1] || Color == IPtr[index - 1 + NX] || Color == IPtr[index + 1 + 3 * NX] || Color == IPtr[index + 2 + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_16(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_16(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + NX] < 13)
                IPtr[index + NX] = (BYTE)Color;
            if (IPtr[index + 1 + 2 * NX] < 13)
                IPtr[index + 1 + 2 * NX] = (BYTE)Color;
            if (IPtr[index + 2 + 2 * NX] < 13)
                IPtr[index + 2 + 2 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT4B(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_16(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_16(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 14)
    {
        if (i <= 1 || i == LY - TY -1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j >= LX-1 || IPtr[index - NX] == IPtr[index + 1] || IPtr[index + NX] == IPtr[index + 1])
            {
                Color = 0;
                while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                    Color++;
            }
            else
                Color = IPtr[index + 1];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_16(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_16(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 13)
    {
        if (j >= LX-2 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                Color++;
            CreateUNIT(fin, MakeXY_16(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (i <= 1 || j <= TX - 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i == LY - TY - 1 && j == TX - 2)
            {
                if (j >= LX - 1 || IPtr[index - 2 * NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (i == LY - TY - 1)
            {
                if (j <= 0 || IPtr[index - 2 * NX] == IPtr[index + 1] || IPtr[index - NX - 3] == IPtr[index + 1] || IPtr[index - 2 * NX - 3] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX] || Color == IPtr[index - NX - 3] || Color == IPtr[index - 2 * NX - 3])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (j == TX - 2)
            {
                if (j <= 0 || IPtr[index - 2 * NX] == IPtr[index + 1] || IPtr[index + NX] == IPtr[index + 1] || IPtr[index + NX - 1] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX] || Color == IPtr[index + NX] || Color == IPtr[index + NX - 1])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else
            {
                if (j <= 0 || IPtr[index - 2 * NX] == IPtr[index + 1] || IPtr[index + NX] == IPtr[index + 1] || IPtr[index + NX - 1] == IPtr[index + 1] || IPtr[index - NX - 3] == IPtr[index + 1] || IPtr[index - 2 * NX - 3] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX] || Color == IPtr[index + NX] || Color == IPtr[index + NX - 1] || Color == IPtr[index - NX - 3] || Color == IPtr[index - 2 * NX - 3])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_16(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_16(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - 1] < 13)
                IPtr[index - 1] = (BYTE)Color;
            if (IPtr[index - NX - 2] < 13)
                IPtr[index - NX - 2] = (BYTE)Color;
            if (IPtr[index - 2 * NX - 2] < 13)
                IPtr[index - 2 * NX - 2] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT5A(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_4(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_5(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j <= 3 || j == LX - 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i >= LY - 1 || IPtr[index - 1] == IPtr[index + NX] || IPtr[index + 1] == IPtr[index + NX])
            {
                Color = 0;
                while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                    Color++;
            }
            else
                Color = IPtr[index + NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_5(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_4(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 14)
    {
        if (i >= LY - 4 || i == 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_4(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (j <= 3 || i <= 4)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j == LX - 2 && i == 5)
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (j == LX - 2)
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX] 
                    || IPtr[index - 1 - 5 * NX] == IPtr[index + NX] || IPtr[index - 2 - 5 * NX] == IPtr[index + NX]
                    || IPtr[index - 3 - 5 * NX] == IPtr[index + NX] || IPtr[index - 4 - 5 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4] 
                        || Color == IPtr[index - 1 - 5 * NX] || Color == IPtr[index - 2 - 5 * NX]
                        || Color == IPtr[index - 3 - 5 * NX] || Color == IPtr[index - 4 - 5 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (i == 5)
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX]
                    || IPtr[index + 1] == IPtr[index + NX] || IPtr[index + 1 - NX] == IPtr[index + NX]
                    || IPtr[index + 1 - 2 * NX] == IPtr[index + NX] || IPtr[index + 1 - 3 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 - NX]
                        || Color == IPtr[index + 1 - 2 * NX] || Color == IPtr[index + 1 - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX] 
                    || IPtr[index - 1 - 5 * NX] == IPtr[index + NX] || IPtr[index - 2 - 5 * NX] == IPtr[index + NX]
                    || IPtr[index - 3 - 5 * NX] == IPtr[index + NX] || IPtr[index - 4 - 5 * NX] == IPtr[index + NX]
                    || IPtr[index + 1] == IPtr[index + NX] || IPtr[index + 1 - NX] == IPtr[index + NX]
                    || IPtr[index + 1 - 2 * NX] == IPtr[index + NX] || IPtr[index + 1 - 3 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4] 
                        || Color == IPtr[index - 1 - 5 * NX] || Color == IPtr[index - 2 - 5 * NX]
                        || Color == IPtr[index - 3 - 5 * NX] || Color == IPtr[index - 4 - 5 * NX]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 - NX]
                        || Color == IPtr[index + 1 - 2 * NX] || Color == IPtr[index + 1 - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_5(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_4(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - NX] < 13)
                IPtr[index - NX] = (BYTE)Color;
            if (IPtr[index - 2*NX] < 13)
                IPtr[index - 2*NX] = (BYTE)Color;
            if (IPtr[index - 3*NX] < 13)
                IPtr[index - 3*NX] = (BYTE)Color;
            if (IPtr[index - 1 - 4 * NX] < 13)
                IPtr[index - 1 - 4 * NX] = (BYTE)Color;
            if (IPtr[index - 2 - 4 * NX] < 13)
                IPtr[index - 2 - 4 * NX] = (BYTE)Color;
            if (IPtr[index - 3 - 4 * NX] < 13)
                IPtr[index - 3 - 4 * NX] = (BYTE)Color;
            if (IPtr[index - 4 - 4 * NX] < 13)
                IPtr[index - 4 - 4 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT5B(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_4(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_5(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 14)
    {
        if (i >= LY-4 || i == 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j <= 0 || IPtr[index + NX] == IPtr[index - 1] || IPtr[index - NX] == IPtr[index - 1])
            {
                Color = 0;
                while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                    Color++;
            }
            else
                Color = IPtr[index - 1];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_5(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_4(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 13)
    {
        if (j >= 3 || j == LX-2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                Color++;
            CreateUNIT(fin, MakeXY_4(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (i >= LY-4 || j >= LX-5)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i == 1 && j == LX-6)
            {
                if (j <= 0 || IPtr[index + 4*NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4*NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (i == 1)
            {
                if (j <= 0 || IPtr[index + 4 * NX] == IPtr[index - 1]
                    || IPtr[index + NX + 5] == IPtr[index - 1] || IPtr[index + 2*NX + 5] == IPtr[index - 1]
                    || IPtr[index + 3*NX + 5] == IPtr[index - 1] || IPtr[index + 4*NX + 5] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4 * NX]
                        || Color == IPtr[index + NX + 5] || Color == IPtr[index + 2 * NX + 5]
                        || Color == IPtr[index + 3 * NX + 5] || Color == IPtr[index + 4 * NX + 5])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (j == LX - 6)
            {
                if (j <= 0 || IPtr[index + 4 * NX] == IPtr[index - 1]
                    || IPtr[index - NX] == IPtr[index - 1] || IPtr[index - NX + 1] == IPtr[index - 1]
                    || IPtr[index - NX + 2] == IPtr[index - 1] || IPtr[index - NX + 3] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4 * NX]
                        || Color == IPtr[index - NX] || Color == IPtr[index - NX + 1]
                        || Color == IPtr[index - NX + 2] || Color == IPtr[index - NX + 3])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else
            {
                if (j <= 0 || IPtr[index + 4 * NX] == IPtr[index - 1]
                    || IPtr[index + NX + 5] == IPtr[index - 1] || IPtr[index + 2 * NX + 5] == IPtr[index - 1]
                    || IPtr[index + 3 * NX + 5] == IPtr[index - 1] || IPtr[index + 4 * NX + 5] == IPtr[index - 1]
                    || IPtr[index - NX] == IPtr[index - 1] || IPtr[index - NX + 1] == IPtr[index - 1]
                    || IPtr[index - NX + 2] == IPtr[index - 1] || IPtr[index - NX + 3] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4 * NX]
                        || Color == IPtr[index + NX + 5] || Color == IPtr[index + 2 * NX + 5]
                        || Color == IPtr[index + 3 * NX + 5] || Color == IPtr[index + 4 * NX + 5]
                        || Color == IPtr[index - NX] || Color == IPtr[index - NX + 1]
                        || Color == IPtr[index - NX + 2] || Color == IPtr[index - NX + 3])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_5(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_4(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 1] < 13)
                IPtr[index + 1] = (BYTE)Color;
            if (IPtr[index + 2] < 13)
                IPtr[index + 2] = (BYTE)Color;
            if (IPtr[index + 3] < 13)
                IPtr[index + 3] = (BYTE)Color;
            if (IPtr[index + NX + 4] < 13)
                IPtr[index + NX + 4] = (BYTE)Color;
            if (IPtr[index + 2 * NX + 4] < 13)
                IPtr[index + 2 * NX + 4] = (BYTE)Color;
            if (IPtr[index + 3 * NX + 4] < 13)
                IPtr[index + 3 * NX + 4] = (BYTE)Color;
            if (IPtr[index + 4 * NX + 4] < 13)
                IPtr[index + 4 * NX + 4] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT6A(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_9(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_0(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j >= LX-4 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i >= LY - 1 || IPtr[index + 1] == IPtr[index + NX] || IPtr[index - 1] == IPtr[index + NX])
            {
                Color = 0;
                while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                    Color++;
            }
            else
                Color = IPtr[index + NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_0(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_9(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 14)
    {
        if (i >= LY - 4 || i == 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_9(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (j >= LX-4 || i <= 4)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j == TX && i == 5)
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (j == TX)
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX]
                    || IPtr[index + 1 - 5 * NX] == IPtr[index + NX] || IPtr[index + 2 - 5 * NX] == IPtr[index + NX]
                    || IPtr[index + 3 - 5 * NX] == IPtr[index + NX] || IPtr[index + 4 - 5 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 - 5 * NX] || Color == IPtr[index + 2 - 5 * NX]
                        || Color == IPtr[index + 3 - 5 * NX] || Color == IPtr[index + 4 - 5 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (i == 5)
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX]
                    || IPtr[index - 1] == IPtr[index + NX] || IPtr[index - 1 - NX] == IPtr[index + NX]
                    || IPtr[index - 1 - 2 * NX] == IPtr[index + NX] || IPtr[index - 1 - 3 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 - NX]
                        || Color == IPtr[index - 1 - 2 * NX] || Color == IPtr[index - 1 - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX]
                    || IPtr[index + 1 - 5 * NX] == IPtr[index + NX] || IPtr[index + 2 - 5 * NX] == IPtr[index + NX]
                    || IPtr[index + 3 - 5 * NX] == IPtr[index + NX] || IPtr[index + 4 - 5 * NX] == IPtr[index + NX]
                    || IPtr[index - 1] == IPtr[index + NX] || IPtr[index - 1 - NX] == IPtr[index + NX]
                    || IPtr[index - 1 - 2 * NX] == IPtr[index + NX] || IPtr[index - 1 - 3 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 - 5 * NX] || Color == IPtr[index + 2 - 5 * NX]
                        || Color == IPtr[index + 3 - 5 * NX] || Color == IPtr[index + 4 - 5 * NX]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 - NX]
                        || Color == IPtr[index - 1 - 2 * NX] || Color == IPtr[index - 1 - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_0(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_9(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - NX] < 13)
                IPtr[index - NX] = (BYTE)Color;
            if (IPtr[index - 2 * NX] < 13)
                IPtr[index - 2 * NX] = (BYTE)Color;
            if (IPtr[index - 3 * NX] < 13)
                IPtr[index - 3 * NX] = (BYTE)Color;
            if (IPtr[index + 1 - 4 * NX] < 13)
                IPtr[index + 1 - 4 * NX] = (BYTE)Color;
            if (IPtr[index + 2 - 4 * NX] < 13)
                IPtr[index + 2 - 4 * NX] = (BYTE)Color;
            if (IPtr[index + 3 - 4 * NX] < 13)
                IPtr[index + 3 - 4 * NX] = (BYTE)Color;
            if (IPtr[index + 4 - 4 * NX] < 13)
                IPtr[index + 4 - 4 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT6B(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_9(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_0(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 14)
    {
        if (i >= LY - 4 || i == 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j >= LX-1 || IPtr[index + NX] == IPtr[index + 1] || IPtr[index - NX] == IPtr[index + 1])
            {
                Color = 0;
                while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                    Color++;
            }
            else
                Color = IPtr[index + 1];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_0(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_9(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 13)
    {
        if (j <= LX-4 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                Color++;
            CreateUNIT(fin, MakeXY_9(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (i >= LY - 4 || j <= TX - 3)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i == 1 && j == TX - 4)
            {
                if (j >= LX-1 || IPtr[index + 4 * NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (i == 1)
            {
                if (j <= 0 || IPtr[index + 4 * NX] == IPtr[index + 1]
                    || IPtr[index + NX - 5] == IPtr[index + 1] || IPtr[index + 2 * NX - 5] == IPtr[index + 1]
                    || IPtr[index + 3 * NX - 5] == IPtr[index + 1] || IPtr[index + 4 * NX - 5] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4 * NX]
                        || Color == IPtr[index + NX - 5] || Color == IPtr[index + 2 * NX - 5]
                        || Color == IPtr[index + 3 * NX - 5] || Color == IPtr[index + 4 * NX - 5])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (j == TX - 4)
            {
                if (j <= 0 || IPtr[index + 4 * NX] == IPtr[index + 1]
                    || IPtr[index - NX] == IPtr[index + 1] || IPtr[index - NX - 1] == IPtr[index + 1]
                    || IPtr[index - NX - 2] == IPtr[index + 1] || IPtr[index - NX - 3] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4 * NX]
                        || Color == IPtr[index - NX] || Color == IPtr[index - NX - 1]
                        || Color == IPtr[index - NX - 2] || Color == IPtr[index - NX - 3])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else
            {
                if (j <= 0 || IPtr[index + 4 * NX] == IPtr[index + 1]
                    || IPtr[index + NX - 5] == IPtr[index + 1] || IPtr[index + 2 * NX - 5] == IPtr[index + 1]
                    || IPtr[index + 3 * NX - 5] == IPtr[index + 1] || IPtr[index + 4 * NX - 5] == IPtr[index + 1]
                    || IPtr[index - NX] == IPtr[index + 1] || IPtr[index - NX - 1] == IPtr[index + 1]
                    || IPtr[index - NX - 2] == IPtr[index + 1] || IPtr[index - NX - 3] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4 * NX]
                        || Color == IPtr[index + NX - 5] || Color == IPtr[index + 2 * NX - 5]
                        || Color == IPtr[index + 3 * NX - 5] || Color == IPtr[index + 4 * NX - 5]
                        || Color == IPtr[index - NX] || Color == IPtr[index - NX - 1]
                        || Color == IPtr[index - NX - 2] || Color == IPtr[index - NX - 3])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_0(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_9(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - 1] < 13)
                IPtr[index - 1] = (BYTE)Color;
            if (IPtr[index - 2] < 13)
                IPtr[index - 2] = (BYTE)Color;
            if (IPtr[index - 3] < 13)
                IPtr[index - 3] = (BYTE)Color;
            if (IPtr[index + NX - 4] < 13)
                IPtr[index + NX - 4] = (BYTE)Color;
            if (IPtr[index + 2 * NX - 4] < 13)
                IPtr[index + 2 * NX - 4] = (BYTE)Color;
            if (IPtr[index + 3 * NX - 4] < 13)
                IPtr[index + 3 * NX - 4] = (BYTE)Color;
            if (IPtr[index + 4 * NX - 4] < 13)
                IPtr[index + 4 * NX - 4] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT7A(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_14(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_15(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j <= 3 || j == LX - 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i <= 0 || IPtr[index - 1] == IPtr[index - NX] || IPtr[index + 1] == IPtr[index - NX])
            {
                Color = 0;
                while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                    Color++;
            }
            else
                Color = IPtr[index - NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_15(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_14(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 14)
    {
        if (i <= 3 || i == LY-TY-1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_14(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (j <= 3 || i >= LY-TY-4)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j == LX - 2 && i == LY-TY-5)
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (j == LX - 2)
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX]
                    || IPtr[index - 1 + 5 * NX] == IPtr[index - NX] || IPtr[index - 2 + 5 * NX] == IPtr[index - NX]
                    || IPtr[index - 3 + 5 * NX] == IPtr[index - NX] || IPtr[index - 4 + 5 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index - 1 + 5 * NX] || Color == IPtr[index - 2 + 5 * NX]
                        || Color == IPtr[index - 3 + 5 * NX] || Color == IPtr[index - 4 + 5 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (i == LY - TY - 5)
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX]
                    || IPtr[index + 1] == IPtr[index - NX] || IPtr[index + 1 + NX] == IPtr[index - NX]
                    || IPtr[index + 1 + 2 * NX] == IPtr[index - NX] || IPtr[index + 1 + 3 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 + NX]
                        || Color == IPtr[index + 1 + 2 * NX] || Color == IPtr[index + 1 + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX]
                    || IPtr[index - 1 + 5 * NX] == IPtr[index - NX] || IPtr[index - 2 + 5 * NX] == IPtr[index - NX]
                    || IPtr[index - 3 + 5 * NX] == IPtr[index - NX] || IPtr[index - 4 + 5 * NX] == IPtr[index - NX]
                    || IPtr[index + 1] == IPtr[index - NX] || IPtr[index + 1 + NX] == IPtr[index - NX]
                    || IPtr[index + 1 + 2 * NX] == IPtr[index - NX] || IPtr[index + 1 + 3 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index - 1 + 5 * NX] || Color == IPtr[index - 2 + 5 * NX]
                        || Color == IPtr[index - 3 + 5 * NX] || Color == IPtr[index - 4 + 5 * NX]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 + NX]
                        || Color == IPtr[index + 1 + 2 * NX] || Color == IPtr[index + 1 + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_15(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_14(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + NX] < 13)
                IPtr[index + NX] = (BYTE)Color;
            if (IPtr[index + 2 * NX] < 13)
                IPtr[index + 2 * NX] = (BYTE)Color;
            if (IPtr[index + 3 * NX] < 13)
                IPtr[index + 3 * NX] = (BYTE)Color;
            if (IPtr[index - 1 + 4 * NX] < 13)
                IPtr[index - 1 + 4 * NX] = (BYTE)Color;
            if (IPtr[index - 2 + 4 * NX] < 13)
                IPtr[index - 2 + 4 * NX] = (BYTE)Color;
            if (IPtr[index - 3 + 4 * NX] < 13)
                IPtr[index - 3 + 4 * NX] = (BYTE)Color;
            if (IPtr[index - 4 + 4 * NX] < 13)
                IPtr[index - 4 + 4 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT7B(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_14(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_15(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 14)
    {
        if (i <= 3 || i == LY-TY-1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j <= 0 || IPtr[index - NX] == IPtr[index - 1] || IPtr[index + NX] == IPtr[index - 1])
            {
                Color = 0;
                while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                    Color++;
            }
            else
                Color = IPtr[index - 1];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_15(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_14(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 13)
    {
        if (j >= 3 || j == LX - 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                Color++;
            CreateUNIT(fin, MakeXY_14(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (i <= 3 || j >= LX - 5)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i == LY-TY-1 && j == LX - 6)
            {
                if (j <= 0 || IPtr[index - 4 * NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (i == LY - TY - 1)
            {
                if (j <= 0 || IPtr[index - 4 * NX] == IPtr[index - 1]
                    || IPtr[index - NX + 5] == IPtr[index - 1] || IPtr[index - 2 * NX + 5] == IPtr[index - 1]
                    || IPtr[index - 3 * NX + 5] == IPtr[index - 1] || IPtr[index - 4 * NX + 5] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4 * NX]
                        || Color == IPtr[index - NX + 5] || Color == IPtr[index - 2 * NX + 5]
                        || Color == IPtr[index - 3 * NX + 5] || Color == IPtr[index - 4 * NX + 5])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (j == LX - 6)
            {
                if (j <= 0 || IPtr[index - 4 * NX] == IPtr[index - 1]
                    || IPtr[index + NX] == IPtr[index - 1] || IPtr[index + NX + 1] == IPtr[index - 1]
                    || IPtr[index + NX + 2] == IPtr[index - 1] || IPtr[index + NX + 3] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4 * NX]
                        || Color == IPtr[index + NX] || Color == IPtr[index + NX + 1]
                        || Color == IPtr[index + NX + 2] || Color == IPtr[index + NX + 3])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else
            {
                if (j <= 0 || IPtr[index - 4 * NX] == IPtr[index - 1]
                    || IPtr[index - NX + 5] == IPtr[index - 1] || IPtr[index - 2 * NX + 5] == IPtr[index - 1]
                    || IPtr[index - 3 * NX + 5] == IPtr[index - 1] || IPtr[index - 4 * NX + 5] == IPtr[index - 1]
                    || IPtr[index + NX] == IPtr[index - 1] || IPtr[index + NX + 1] == IPtr[index - 1]
                    || IPtr[index + NX + 2] == IPtr[index - 1] || IPtr[index + NX + 3] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4 * NX]
                        || Color == IPtr[index - NX + 5] || Color == IPtr[index - 2 * NX + 5]
                        || Color == IPtr[index - 3 * NX + 5] || Color == IPtr[index - 4 * NX + 5]
                        || Color == IPtr[index + NX] || Color == IPtr[index + NX + 1]
                        || Color == IPtr[index + NX + 2] || Color == IPtr[index + NX + 3])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_15(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_14(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 1] < 13)
                IPtr[index + 1] = (BYTE)Color;
            if (IPtr[index + 2] < 13)
                IPtr[index + 2] = (BYTE)Color;
            if (IPtr[index + 3] < 13)
                IPtr[index + 3] = (BYTE)Color;
            if (IPtr[index - NX + 4] < 13)
                IPtr[index - NX + 4] = (BYTE)Color;
            if (IPtr[index - 2 * NX + 4] < 13)
                IPtr[index - 2 * NX + 4] = (BYTE)Color;
            if (IPtr[index - 3 * NX + 4] < 13)
                IPtr[index - 3 * NX + 4] = (BYTE)Color;
            if (IPtr[index - 4 * NX + 4] < 13)
                IPtr[index - 4 * NX + 4] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT8A(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_19(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_10(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j >= LX-4 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i <= 0 || IPtr[index + 1] == IPtr[index - NX] || IPtr[index - 1] == IPtr[index - NX])
            {
                Color = 0;
                while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                    Color++;
            }
            else
                Color = IPtr[index - NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_10(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_19(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 14)
    {
        if (i <= 3 || i == LY - TY - 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_19(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (j >= LX-4 || i >= LY - TY - 4)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j == TX && i == LY - TY - 5)
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (j == TX)
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX]
                    || IPtr[index + 1 + 5 * NX] == IPtr[index - NX] || IPtr[index + 2 + 5 * NX] == IPtr[index - NX]
                    || IPtr[index + 3 + 5 * NX] == IPtr[index - NX] || IPtr[index + 4 + 5 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 + 5 * NX] || Color == IPtr[index + 2 + 5 * NX]
                        || Color == IPtr[index + 3 + 5 * NX] || Color == IPtr[index + 4 + 5 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (i == LY - TY - 5)
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX]
                    || IPtr[index - 1] == IPtr[index - NX] || IPtr[index - 1 + NX] == IPtr[index - NX]
                    || IPtr[index - 1 + 2 * NX] == IPtr[index - NX] || IPtr[index - 1 + 3 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 + NX]
                        || Color == IPtr[index - 1 + 2 * NX] || Color == IPtr[index - 1 + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX]
                    || IPtr[index + 1 + 5 * NX] == IPtr[index - NX] || IPtr[index + 2 + 5 * NX] == IPtr[index - NX]
                    || IPtr[index + 3 + 5 * NX] == IPtr[index - NX] || IPtr[index + 4 + 5 * NX] == IPtr[index - NX]
                    || IPtr[index - 1] == IPtr[index - NX] || IPtr[index - 1 + NX] == IPtr[index - NX]
                    || IPtr[index - 1 + 2 * NX] == IPtr[index - NX] || IPtr[index - 1 + 3 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 + 5 * NX] || Color == IPtr[index + 2 + 5 * NX]
                        || Color == IPtr[index + 3 + 5 * NX] || Color == IPtr[index + 4 + 5 * NX]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 + NX]
                        || Color == IPtr[index - 1 + 2 * NX] || Color == IPtr[index - 1 + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_10(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_19(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + NX] < 13)
                IPtr[index + NX] = (BYTE)Color;
            if (IPtr[index + 2 * NX] < 13)
                IPtr[index + 2 * NX] = (BYTE)Color;
            if (IPtr[index + 3 * NX] < 13)
                IPtr[index + 3 * NX] = (BYTE)Color;
            if (IPtr[index + 1 + 4 * NX] < 13)
                IPtr[index + 1 + 4 * NX] = (BYTE)Color;
            if (IPtr[index + 2 + 4 * NX] < 13)
                IPtr[index + 2 + 4 * NX] = (BYTE)Color;
            if (IPtr[index + 3 + 4 * NX] < 13)
                IPtr[index + 3 + 4 * NX] = (BYTE)Color;
            if (IPtr[index + 4 + 4 * NX] < 13)
                IPtr[index + 4 + 4 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT8B(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_19(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_10(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 14)
    {
        if (i <= 3 || i == LY - TY - 1)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (j >= LX-1 || IPtr[index - NX] == IPtr[index + 1] || IPtr[index + NX] == IPtr[index + 1])
            {
                Color = 0;
                while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                    Color++;
            }
            else
                Color = IPtr[index + 1];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_10(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_19(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 1;
        }
    }
    else if (BTemp == 13)
    {
        if (j <= LX-4 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            Color = 0;
            while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                Color++;
            CreateUNIT(fin, MakeXY_19(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        if (i <= 3 || j <= TX-3)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i == LY - TY - 1 && j == TX-4)
            {
                if (j <= 0 || IPtr[index - 4 * NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (i == LY - TY - 1)
            {
                if (j <= 0 || IPtr[index - 4 * NX] == IPtr[index + 1]
                    || IPtr[index - NX - 5] == IPtr[index + 1] || IPtr[index - 2 * NX - 5] == IPtr[index + 1]
                    || IPtr[index - 3 * NX - 5] == IPtr[index + 1] || IPtr[index - 4 * NX - 5] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4 * NX]
                        || Color == IPtr[index - NX - 5] || Color == IPtr[index - 2 * NX - 5]
                        || Color == IPtr[index - 3 * NX - 5] || Color == IPtr[index - 4 * NX - 5])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (j == TX-4)
            {
                if (j <= 0 || IPtr[index - 4 * NX] == IPtr[index + 1]
                    || IPtr[index + NX] == IPtr[index + 1] || IPtr[index + NX - 1] == IPtr[index + 1]
                    || IPtr[index + NX - 2] == IPtr[index + 1] || IPtr[index + NX - 3] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4 * NX]
                        || Color == IPtr[index + NX] || Color == IPtr[index + NX - 1]
                        || Color == IPtr[index + NX - 2] || Color == IPtr[index + NX - 3])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else
            {
                if (j <= 0 || IPtr[index - 4 * NX] == IPtr[index + 1]
                    || IPtr[index - NX - 5] == IPtr[index + 1] || IPtr[index - 2 * NX - 5] == IPtr[index + 1]
                    || IPtr[index - 3 * NX - 5] == IPtr[index + 1] || IPtr[index - 4 * NX - 5] == IPtr[index + 1]
                    || IPtr[index + NX] == IPtr[index + 1] || IPtr[index + NX - 1] == IPtr[index + 1]
                    || IPtr[index + NX - 2] == IPtr[index + 1] || IPtr[index + NX - 3] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4 * NX]
                        || Color == IPtr[index - NX - 5] || Color == IPtr[index - 2 * NX - 5]
                        || Color == IPtr[index - 3 * NX - 5] || Color == IPtr[index - 4 * NX - 5]
                        || Color == IPtr[index + NX] || Color == IPtr[index + NX - 1]
                        || Color == IPtr[index + NX - 2] || Color == IPtr[index + NX - 3])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_10(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_19(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - 1] < 13)
                IPtr[index - 1] = (BYTE)Color;
            if (IPtr[index - 2] < 13)
                IPtr[index - 2] = (BYTE)Color;
            if (IPtr[index - 3] < 13)
                IPtr[index - 3] = (BYTE)Color;
            if (IPtr[index - NX - 4] < 13)
                IPtr[index - NX - 4] = (BYTE)Color;
            if (IPtr[index - 2 * NX - 4] < 13)
                IPtr[index - 2 * NX - 4] = (BYTE)Color;
            if (IPtr[index - 3 * NX - 4] < 13)
                IPtr[index - 3 * NX - 4] = (BYTE)Color;
            if (IPtr[index - 4 * NX - 4] < 13)
                IPtr[index - 4 * NX - 4] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT9A(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_3(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_2(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j <= 3 || j == LX - 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i >= LY - 1 || IPtr[index - 1] == IPtr[index + NX] || IPtr[index + 1] == IPtr[index + NX])
            {
                Color = 0;
                while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                    Color++;
            }
            else
                Color = IPtr[index + NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_2(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_3(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - 4] == 12)
                return 4;
            else
                return 1;
        }
    }
    else if (BTemp == 14)
    {
        int index = i * NX + j;
        if (i >= LY - 2 || i == 1)
            return 0;
        else if (IPtr[index + 2 * NX] == 12)
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_3(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 5;
        }
        else if (i >= LY - 3)
            return 0;
        else
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_3(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        int index = i * NX + j;
        if (j <= 3 || i <= 2)
            return 0;
        else if (IPtr[index - 4] == 12)
        {
            DWORD Color;
            if (j == LX - 2 && i == 3)
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (j == LX - 2)
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX]
                    || IPtr[index - 1 - 3 * NX] == IPtr[index + NX] || IPtr[index - 2 - 3 * NX] == IPtr[index + NX]
                    || IPtr[index - 3 - 3 * NX] == IPtr[index + NX] || IPtr[index - 4 - 3 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index - 1 - 3 * NX] || Color == IPtr[index - 2 - 3 * NX]
                        || Color == IPtr[index - 3 - 3 * NX] || Color == IPtr[index - 4 - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (i == 3)
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX]
                    || IPtr[index + 1] == IPtr[index + NX] || IPtr[index + 1 - NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX]
                    || IPtr[index - 1 - 3 * NX] == IPtr[index + NX] || IPtr[index - 2 - 3 * NX] == IPtr[index + NX]
                    || IPtr[index - 3 - 3 * NX] == IPtr[index + NX] || IPtr[index - 4 - 3 * NX] == IPtr[index + NX]
                    || IPtr[index + 1] == IPtr[index + NX] || IPtr[index + 1 - NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index - 1 - 3 * NX] || Color == IPtr[index - 2 - 3 * NX]
                        || Color == IPtr[index - 3 - 3 * NX] || Color == IPtr[index - 4 - 3 * NX]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_2(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_3(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - NX] < 13)
                IPtr[index - NX] = (BYTE)Color;
            if (IPtr[index - 1 - 2 * NX] < 13)
                IPtr[index - 1 - 2 * NX] = (BYTE)Color;
            if (IPtr[index - 2 - 2 * NX] < 13)
                IPtr[index - 2 - 2 * NX] = (BYTE)Color;
            if (IPtr[index - 3 - 2 * NX] < 13)
                IPtr[index - 3 - 2 * NX] = (BYTE)Color;
            if (IPtr[index - 4 - 2 * NX] < 13)
                IPtr[index - 4 - 2 * NX] = (BYTE)Color;
            return 6;
        }
        else if (i <= 3)
            return 0;
        else
        {
            DWORD Color;
            if (j == LX - 2 && i == 4)
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (j == LX - 2)
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX]
                    || IPtr[index - 1 - 4 * NX] == IPtr[index + NX] || IPtr[index - 2 - 4 * NX] == IPtr[index + NX]
                    || IPtr[index - 3 - 4 * NX] == IPtr[index + NX] || IPtr[index - 4 - 4 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index - 1 - 4 * NX] || Color == IPtr[index - 2 - 4 * NX]
                        || Color == IPtr[index - 3 - 4 * NX] || Color == IPtr[index - 4 - 4 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (i == 4)
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX]
                    || IPtr[index + 1] == IPtr[index + NX] || IPtr[index + 1 - NX] == IPtr[index + NX]
                    || IPtr[index + 1 - 2 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 - NX]
                        || Color == IPtr[index + 1 - 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else
            {
                if (i >= LY - 1 || IPtr[index - 4] == IPtr[index + NX]
                    || IPtr[index - 1 - 4 * NX] == IPtr[index + NX] || IPtr[index - 2 - 4 * NX] == IPtr[index + NX]
                    || IPtr[index - 3 - 4 * NX] == IPtr[index + NX] || IPtr[index - 4 - 4 * NX] == IPtr[index + NX]
                    || IPtr[index + 1] == IPtr[index + NX] || IPtr[index + 1 - NX] == IPtr[index + NX]
                    || IPtr[index + 1 - 2 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index - 1 - 4 * NX] || Color == IPtr[index - 2 - 4 * NX]
                        || Color == IPtr[index - 3 - 4 * NX] || Color == IPtr[index - 4 - 4 * NX]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 - NX]
                        || Color == IPtr[index + 1 - 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_2(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_3(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - NX] < 13)
                IPtr[index - NX] = (BYTE)Color;
            if (IPtr[index - 2 * NX] < 13)
                IPtr[index - 2 * NX] = (BYTE)Color;
            if (IPtr[index - 1 - 3 * NX] < 13)
                IPtr[index - 1 - 3 * NX] = (BYTE)Color;
            if (IPtr[index - 2 - 3 * NX] < 13)
                IPtr[index - 2 - 3 * NX] = (BYTE)Color;
            if (IPtr[index - 3 - 3 * NX] < 13)
                IPtr[index - 3 - 3 * NX] = (BYTE)Color;
            if (IPtr[index - 4 - 3 * NX] < 13)
                IPtr[index - 4 - 3 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT9B(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_3(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_2(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j <= 3 || j == LX - 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i >= LY - 1 || IPtr[index - 1] == IPtr[index + NX] || IPtr[index + 1] == IPtr[index + NX])
            {
                Color = 0;
                while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                    Color++;
            }
            else
                Color = IPtr[index + NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_2(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_3(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - 4] == 12)
                return 4;
            else
                return 1;
        }
    }
    else if (BTemp == 14)
    {
        int index = i * NX + j;
        if (i >= LY - 2 || i == 1)
            return 0;
        else if (IPtr[index + 2 * NX] == 12)
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_3(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 5;
        }
        else if (i >= LY - 3)
            return 0;
        else
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_3(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        int index = i * NX + j;
        if (i >= LY-2 || j >= LX-5)
            return 0;
        else if (IPtr[index + 2*NX] == 12)
        {
            DWORD Color;
            if (i == 1 && j == LX-6)
            {
                if (j <= 0 || IPtr[index + 2*NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2*NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (i == 1)
            {
                if (j <= 0 || IPtr[index + 2 * NX] == IPtr[index - 1]
                    || IPtr[index + NX + 5] == IPtr[index - 1] || IPtr[index + 2*NX + 5] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX]
                        || Color == IPtr[index + NX + 5] || Color == IPtr[index + 2 * NX + 5])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (j == LX-6)
            {
                if (j <= 0 || IPtr[index + 2 * NX] == IPtr[index - 1]
                    || IPtr[index - NX] == IPtr[index - 1] || IPtr[index + 1 - NX] == IPtr[index - 1]
                    || IPtr[index + 2 - NX] == IPtr[index - 1] || IPtr[index + 3 - NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX]
                        || Color == IPtr[index - NX] || Color == IPtr[index + 1 - NX]
                        || Color == IPtr[index + 2 - NX] || Color == IPtr[index + 3 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else
            {
                if (j <= 0 || IPtr[index + 2 * NX] == IPtr[index - 1]
                    || IPtr[index + NX + 5] == IPtr[index - 1] || IPtr[index + 2 * NX + 5] == IPtr[index - 1]
                    || IPtr[index - NX] == IPtr[index - 1] || IPtr[index + 1 - NX] == IPtr[index - 1]
                    || IPtr[index + 2 - NX] == IPtr[index - 1] || IPtr[index + 3 - NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX]
                        || Color == IPtr[index + NX + 5] || Color == IPtr[index + 2 * NX + 5]
                        || Color == IPtr[index - NX] || Color == IPtr[index + 1 - NX]
                        || Color == IPtr[index + 2 - NX] || Color == IPtr[index + 3 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_2(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_3(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 1] < 13)
                IPtr[index + 1] = (BYTE)Color;
            if (IPtr[index + 2] < 13)
                IPtr[index + 2] = (BYTE)Color;
            if (IPtr[index + 3] < 13)
                IPtr[index + 3] = (BYTE)Color;
            if (IPtr[index + 4 + NX] < 13)
                IPtr[index + 4 + NX] = (BYTE)Color;
            if (IPtr[index + 4 + 2 * NX] < 13)
                IPtr[index + 4 + 2 * NX] = (BYTE)Color;
            return 6;
        }
        else if (i >= LY - 3)
            return 0;
        else
        {
            DWORD Color;
            if (i == 1 && j == LX - 6)
            {
                if (j <= 0 || IPtr[index + 3 * NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (i == 1)
            {
                if (j <= 0 || IPtr[index + 3 * NX] == IPtr[index - 1]
                    || IPtr[index + NX + 5] == IPtr[index - 1] || IPtr[index + 2 * NX + 5] == IPtr[index - 1]
                    || IPtr[index + 3 * NX + 5] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 3 * NX]
                        || Color == IPtr[index + NX + 5] || Color == IPtr[index + 2 * NX + 5]
                        || Color == IPtr[index + 3 * NX + 5])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (j == LX - 6)
            {
                if (j <= 0 || IPtr[index + 3 * NX] == IPtr[index - 1]
                    || IPtr[index - NX] == IPtr[index - 1] || IPtr[index + 1 - NX] == IPtr[index - 1]
                    || IPtr[index + 2 - NX] == IPtr[index - 1] || IPtr[index + 3 - NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 3 * NX]
                        || Color == IPtr[index - NX] || Color == IPtr[index + 1 - NX]
                        || Color == IPtr[index + 2 - NX] || Color == IPtr[index + 3 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else
            {
                if (j <= 0 || IPtr[index + 3 * NX] == IPtr[index - 1]
                    || IPtr[index + NX + 5] == IPtr[index - 1] || IPtr[index + 2 * NX + 5] == IPtr[index - 1]
                    || IPtr[index + 3 * NX + 5] == IPtr[index - 1]
                    || IPtr[index - NX] == IPtr[index - 1] || IPtr[index + 1 - NX] == IPtr[index - 1]
                    || IPtr[index + 2 - NX] == IPtr[index - 1] || IPtr[index + 3 - NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 3 * NX]
                        || Color == IPtr[index + NX + 5] || Color == IPtr[index + 2 * NX + 5]
                        || Color == IPtr[index + 3 * NX + 5]
                        || Color == IPtr[index - NX] || Color == IPtr[index + 1 - NX]
                        || Color == IPtr[index + 2 - NX] || Color == IPtr[index + 3 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_2(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_3(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 1] < 13)
                IPtr[index + 1] = (BYTE)Color;
            if (IPtr[index + 2] < 13)
                IPtr[index + 2] = (BYTE)Color;
            if (IPtr[index + 3] < 13)
                IPtr[index + 3] = (BYTE)Color;
            if (IPtr[index + 4 + NX] < 13)
                IPtr[index + 4 + NX] = (BYTE)Color;
            if (IPtr[index + 4 + 2 * NX] < 13)
                IPtr[index + 4 + 2 * NX] = (BYTE)Color;
            if (IPtr[index + 4 + 3 * NX] < 13)
                IPtr[index + 4 + 3 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT0A(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_8(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_7(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j >= LX-4 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i >= LY - 1 || IPtr[index + 1] == IPtr[index + NX] || IPtr[index - 1] == IPtr[index + NX])
            {
                Color = 0;
                while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                    Color++;
            }
            else
                Color = IPtr[index + NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_7(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_8(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 4] == 12)
                return 4;
            else
                return 1;
        }
    }
    else if (BTemp == 14)
    {
        int index = i * NX + j;
        if (i >= LY - 2 || i == 1)
            return 0;
        else if (IPtr[index + 2 * NX] == 12)
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_8(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 5;
        }
        else if (i >= LY - 3)
            return 0;
        else
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_8(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        int index = i * NX + j;
        if (j >= LX-4 || i <= 2)
            return 0;
        else if (IPtr[index + 4] == 12)
        {
            DWORD Color;
            if (j == TX && i == 3)
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (j == TX)
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX]
                    || IPtr[index + 1 - 3 * NX] == IPtr[index + NX] || IPtr[index + 2 - 3 * NX] == IPtr[index + NX]
                    || IPtr[index + 3 - 3 * NX] == IPtr[index + NX] || IPtr[index + 4 - 3 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 - 3 * NX] || Color == IPtr[index + 2 - 3 * NX]
                        || Color == IPtr[index + 3 - 3 * NX] || Color == IPtr[index + 4 - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (i == 3)
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX]
                    || IPtr[index - 1] == IPtr[index + NX] || IPtr[index - 1 - NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX]
                    || IPtr[index + 1 - 3 * NX] == IPtr[index + NX] || IPtr[index + 2 - 3 * NX] == IPtr[index + NX]
                    || IPtr[index + 3 - 3 * NX] == IPtr[index + NX] || IPtr[index + 4 - 3 * NX] == IPtr[index + NX]
                    || IPtr[index - 1] == IPtr[index + NX] || IPtr[index - 1 - NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 - 3 * NX] || Color == IPtr[index + 2 - 3 * NX]
                        || Color == IPtr[index + 3 - 3 * NX] || Color == IPtr[index + 4 - 3 * NX]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_7(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_8(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - NX] < 13)
                IPtr[index - NX] = (BYTE)Color;
            if (IPtr[index + 1 - 2 * NX] < 13)
                IPtr[index + 1 - 2 * NX] = (BYTE)Color;
            if (IPtr[index + 2 - 2 * NX] < 13)
                IPtr[index + 2 - 2 * NX] = (BYTE)Color;
            if (IPtr[index + 3 - 2 * NX] < 13)
                IPtr[index + 3 - 2 * NX] = (BYTE)Color;
            if (IPtr[index + 4 - 2 * NX] < 13)
                IPtr[index + 4 - 2 * NX] = (BYTE)Color;
            return 6;
        }
        else if (i <= 3)
            return 0;
        else
        {
            DWORD Color;
            if (j == TX && i == 4)
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (j == TX)
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX]
                    || IPtr[index + 1 - 4 * NX] == IPtr[index + NX] || IPtr[index + 2 - 4 * NX] == IPtr[index + NX]
                    || IPtr[index + 3 - 4 * NX] == IPtr[index + NX] || IPtr[index + 4 - 4 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 - 4 * NX] || Color == IPtr[index + 2 - 4 * NX]
                        || Color == IPtr[index + 3 - 4 * NX] || Color == IPtr[index + 4 - 4 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else if (i == 4)
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX]
                    || IPtr[index - 1] == IPtr[index + NX] || IPtr[index - 1 - NX] == IPtr[index + NX]
                    || IPtr[index - 1 - 2 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 - NX]
                        || Color == IPtr[index - 1 - 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }
            else
            {
                if (i >= LY - 1 || IPtr[index + 4] == IPtr[index + NX]
                    || IPtr[index + 1 - 4 * NX] == IPtr[index + NX] || IPtr[index + 2 - 4 * NX] == IPtr[index + NX]
                    || IPtr[index + 3 - 4 * NX] == IPtr[index + NX] || IPtr[index + 4 - 4 * NX] == IPtr[index + NX]
                    || IPtr[index - 1] == IPtr[index + NX] || IPtr[index - 1 - NX] == IPtr[index + NX]
                    || IPtr[index - 1 - 2 * NX] == IPtr[index + NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 - 4 * NX] || Color == IPtr[index + 2 - 4 * NX]
                        || Color == IPtr[index + 3 - 4 * NX] || Color == IPtr[index + 4 - 4 * NX]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 - NX]
                        || Color == IPtr[index - 1 - 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_7(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_8(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - NX] < 13)
                IPtr[index - NX] = (BYTE)Color;
            if (IPtr[index - 2 * NX] < 13)
                IPtr[index - 2 * NX] = (BYTE)Color;
            if (IPtr[index + 1 - 3 * NX] < 13)
                IPtr[index + 1 - 3 * NX] = (BYTE)Color;
            if (IPtr[index + 2 - 3 * NX] < 13)
                IPtr[index + 2 - 3 * NX] = (BYTE)Color;
            if (IPtr[index + 3 - 3 * NX] < 13)
                IPtr[index + 3 - 3 * NX] = (BYTE)Color;
            if (IPtr[index + 4 - 3 * NX] < 13)
                IPtr[index + 4 - 3 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNIT0B(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_8(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_7(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j >= LX-4 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i >= LY - 1 || IPtr[index + 1] == IPtr[index + NX] || IPtr[index - 1] == IPtr[index + NX])
            {
                Color = 0;
                while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                    Color++;
            }
            else
                Color = IPtr[index + NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_7(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_8(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 4] == 12)
                return 4;
            else
                return 1;
        }
    }
    else if (BTemp == 14)
    {
        int index = i * NX + j;
        if (i >= LY - 2 || i == 1)
            return 0;
        else if (IPtr[index + 2 * NX] == 12)
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_8(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 5;
        }
        else if (i >= LY - 3)
            return 0;
        else
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index - NX] || Color == IPtr[index + NX])
                Color++;
            CreateUNIT(fin, MakeXY_8(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        int index = i * NX + j;
        if (i >= LY - 2 || j <= TX-3)
            return 0;
        else if (IPtr[index + 2 * NX] == 12)
        {
            DWORD Color;
            if (i == 1 && j == TX - 4)
            {
                if (j >= LX-1 || IPtr[index + 2 * NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (i == 1)
            {
                if (j >= LX - 1 || IPtr[index + 2 * NX] == IPtr[index + 1]
                    || IPtr[index + NX - 5] == IPtr[index + 1] || IPtr[index + 2 * NX - 5] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX]
                        || Color == IPtr[index + NX - 5] || Color == IPtr[index + 2 * NX - 5])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (j == TX-4)
            {
                if (j >= LX - 1 || IPtr[index + 2 * NX] == IPtr[index + 1]
                    || IPtr[index - NX] == IPtr[index + 1] || IPtr[index - 1 - NX] == IPtr[index + 1]
                    || IPtr[index - 2 - NX] == IPtr[index + 1] || IPtr[index - 3 - NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX]
                        || Color == IPtr[index - NX] || Color == IPtr[index - 1 - NX]
                        || Color == IPtr[index - 2 - NX] || Color == IPtr[index - 3 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else
            {
                if (j >= LX - 1 || IPtr[index + 2 * NX] == IPtr[index + 1]
                    || IPtr[index + NX - 5] == IPtr[index + 1] || IPtr[index + 2 * NX - 5] == IPtr[index + 1]
                    || IPtr[index - NX] == IPtr[index + 1] || IPtr[index - 1 - NX] == IPtr[index + 1]
                    || IPtr[index - 2 - NX] == IPtr[index + 1] || IPtr[index - 3 - NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 2 * NX]
                        || Color == IPtr[index + NX - 5] || Color == IPtr[index + 2 * NX - 5]
                        || Color == IPtr[index - NX] || Color == IPtr[index - 1 - NX]
                        || Color == IPtr[index - 2 - NX] || Color == IPtr[index - 3 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_7(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_8(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - 1] < 13)
                IPtr[index - 1] = (BYTE)Color;
            if (IPtr[index - 2] < 13)
                IPtr[index - 2] = (BYTE)Color;
            if (IPtr[index - 3] < 13)
                IPtr[index - 3] = (BYTE)Color;
            if (IPtr[index - 4 + NX] < 13)
                IPtr[index - 4 + NX] = (BYTE)Color;
            if (IPtr[index - 4 + 2 * NX] < 13)
                IPtr[index - 4 + 2 * NX] = (BYTE)Color;
            return 6;
        }
        else if (i >= LY - 3)
            return 0;
        else
        {
            DWORD Color;
            if (i == 1 && j == TX-4)
            {
                if (j >= LX-1 || IPtr[index + 3 * NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (i == 1)
            {
                if (j >= LX - 1 || IPtr[index + 3 * NX] == IPtr[index + 1]
                    || IPtr[index + NX - 5] == IPtr[index + 1] || IPtr[index + 2 * NX - 5] == IPtr[index + 1]
                    || IPtr[index + 3 * NX - 5] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 3 * NX]
                        || Color == IPtr[index + NX - 5] || Color == IPtr[index + 2 * NX - 5]
                        || Color == IPtr[index + 3 * NX - 5])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (j == TX-4)
            {
                if (j >= LX - 1 || IPtr[index + 3 * NX] == IPtr[index + 1]
                    || IPtr[index - NX] == IPtr[index + 1] || IPtr[index - 1 - NX] == IPtr[index + 1]
                    || IPtr[index - 2 - NX] == IPtr[index + 1] || IPtr[index - 3 - NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 3 * NX]
                        || Color == IPtr[index - NX] || Color == IPtr[index - 1 - NX]
                        || Color == IPtr[index - 2 - NX] || Color == IPtr[index - 3 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else
            {
                if (j >= LX - 1 || IPtr[index + 3 * NX] == IPtr[index + 1]
                    || IPtr[index + NX - 5] == IPtr[index + 1] || IPtr[index + 2 * NX - 5] == IPtr[index + 1]
                    || IPtr[index + 3 * NX - 5] == IPtr[index + 1]
                    || IPtr[index - NX] == IPtr[index + 1] || IPtr[index - 1 - NX] == IPtr[index + 1]
                    || IPtr[index - 2 - NX] == IPtr[index + 1] || IPtr[index - 3 - NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index + 3 * NX]
                        || Color == IPtr[index + NX - 5] || Color == IPtr[index + 2 * NX - 5]
                        || Color == IPtr[index + 3 * NX - 5]
                        || Color == IPtr[index - NX] || Color == IPtr[index - 1 - NX]
                        || Color == IPtr[index - 2 - NX] || Color == IPtr[index - 3 - NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_7(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_8(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - 1] < 13)
                IPtr[index - 1] = (BYTE)Color;
            if (IPtr[index - 2] < 13)
                IPtr[index - 2] = (BYTE)Color;
            if (IPtr[index - 3] < 13)
                IPtr[index - 3] = (BYTE)Color;
            if (IPtr[index - 4 + NX] < 13)
                IPtr[index - 4 + NX] = (BYTE)Color;
            if (IPtr[index - 4 + 2 * NX] < 13)
                IPtr[index - 4 + 2 * NX] = (BYTE)Color;
            if (IPtr[index - 4 + 3 * NX] < 13)
                IPtr[index - 4 + 3 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNITAA(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_13(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_12(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j <= 3 || j == LX - 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i <= 0 || IPtr[index - 1] == IPtr[index - NX] || IPtr[index + 1] == IPtr[index - NX])
            {
                Color = 0;
                while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                    Color++;
            }
            else
                Color = IPtr[index - NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_12(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_13(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - 4] == 12)
                return 4;
            else
                return 1;
        }
    }
    else if (BTemp == 14)
    {
        int index = i * NX + j;
        if (i <= 1 || i == LY-TY-1)
            return 0;
        else if (IPtr[index - 2 * NX] == 12)
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_13(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 5;
        }
        else if (i <= 2)
            return 0;
        else
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_13(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        int index = i * NX + j;
        if (j <= 3 || i >= LY-TY-2)
            return 0;
        else if (IPtr[index - 4] == 12)
        {
            DWORD Color;
            if (j == LX - 2 && i == LY-TY-3)
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (j == LX - 2)
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX]
                    || IPtr[index - 1 + 3 * NX] == IPtr[index - NX] || IPtr[index - 2 + 3 * NX] == IPtr[index - NX]
                    || IPtr[index - 3 + 3 * NX] == IPtr[index - NX] || IPtr[index - 4 + 3 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index - 1 + 3 * NX] || Color == IPtr[index - 2 + 3 * NX]
                        || Color == IPtr[index - 3 + 3 * NX] || Color == IPtr[index - 4 + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (i == LY - TY - 3)
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX]
                    || IPtr[index + 1] == IPtr[index - NX] || IPtr[index + 1 + NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX]
                    || IPtr[index - 1 + 3 * NX] == IPtr[index - NX] || IPtr[index - 2 + 3 * NX] == IPtr[index - NX]
                    || IPtr[index - 3 + 3 * NX] == IPtr[index - NX] || IPtr[index - 4 + 3 * NX] == IPtr[index - NX]
                    || IPtr[index + 1] == IPtr[index - NX] || IPtr[index + 1 + NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index - 1 + 3 * NX] || Color == IPtr[index - 2 + 3 * NX]
                        || Color == IPtr[index - 3 + 3 * NX] || Color == IPtr[index - 4 + 3 * NX]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_12(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_13(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + NX] < 13)
                IPtr[index + NX] = (BYTE)Color;
            if (IPtr[index - 1 + 2 * NX] < 13)
                IPtr[index - 1 + 2 * NX] = (BYTE)Color;
            if (IPtr[index - 2 + 2 * NX] < 13)
                IPtr[index - 2 + 2 * NX] = (BYTE)Color;
            if (IPtr[index - 3 + 2 * NX] < 13)
                IPtr[index - 3 + 2 * NX] = (BYTE)Color;
            if (IPtr[index - 4 + 2 * NX] < 13)
                IPtr[index - 4 + 2 * NX] = (BYTE)Color;
            return 6;
        }
        else if (i >= LY-TY-3)
            return 0;
        else
        {
            DWORD Color;
            if (j == LX - 2 && i == LY-TY-4)
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (j == LX - 2)
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX]
                    || IPtr[index - 1 + 4 * NX] == IPtr[index - NX] || IPtr[index - 2 + 4 * NX] == IPtr[index - NX]
                    || IPtr[index - 3 + 4 * NX] == IPtr[index - NX] || IPtr[index - 4 + 4 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index - 1 + 4 * NX] || Color == IPtr[index - 2 + 4 * NX]
                        || Color == IPtr[index - 3 + 4 * NX] || Color == IPtr[index - 4 + 4 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (i == LY - TY - 4)
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX]
                    || IPtr[index + 1] == IPtr[index - NX] || IPtr[index + 1 + NX] == IPtr[index - NX]
                    || IPtr[index + 1 + 2 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 + NX]
                        || Color == IPtr[index + 1 + 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else
            {
                if (i <= 0 || IPtr[index - 4] == IPtr[index - NX]
                    || IPtr[index - 1 + 4 * NX] == IPtr[index - NX] || IPtr[index - 2 + 4 * NX] == IPtr[index - NX]
                    || IPtr[index - 3 + 4 * NX] == IPtr[index - NX] || IPtr[index - 4 + 4 * NX] == IPtr[index - NX]
                    || IPtr[index + 1] == IPtr[index - NX] || IPtr[index + 1 + NX] == IPtr[index - NX]
                    || IPtr[index + 1 + 2 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index - 4]
                        || Color == IPtr[index - 1 + 4 * NX] || Color == IPtr[index - 2 + 4 * NX]
                        || Color == IPtr[index - 3 + 4 * NX] || Color == IPtr[index - 4 + 4 * NX]
                        || Color == IPtr[index + 1] || Color == IPtr[index + 1 + NX]
                        || Color == IPtr[index + 1 + 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_12(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_13(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + NX] < 13)
                IPtr[index + NX] = (BYTE)Color;
            if (IPtr[index + 2 * NX] < 13)
                IPtr[index + 2 * NX] = (BYTE)Color;
            if (IPtr[index - 1 + 3 * NX] < 13)
                IPtr[index - 1 + 3 * NX] = (BYTE)Color;
            if (IPtr[index - 2 + 3 * NX] < 13)
                IPtr[index - 2 + 3 * NX] = (BYTE)Color;
            if (IPtr[index - 3 + 3 * NX] < 13)
                IPtr[index - 3 + 3 * NX] = (BYTE)Color;
            if (IPtr[index - 4 + 3 * NX] < 13)
                IPtr[index - 4 + 3 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNITAB(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_13(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_12(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j <= 3 || j == LX - 2)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i <= 0 || IPtr[index - 1] == IPtr[index - NX] || IPtr[index + 1] == IPtr[index - NX])
            {
                Color = 0;
                while (Color == IPtr[index - 1] || Color == IPtr[index + 1])
                    Color++;
            }
            else
                Color = IPtr[index - NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_12(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_13(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - 4] == 12)
                return 4;
            else
                return 1;
        }
    }
    else if (BTemp == 14)
    {
        int index = i * NX + j;
        if (i <= 1 || i == LY-TY-1)
            return 0;
        else if (IPtr[index - 2 * NX] == 12)
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_13(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 5;
        }
        else if (i <= 2)
            return 0;
        else
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_13(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        int index = i * NX + j;
        if (i <= 1 || j >= LX - 5)
            return 0;
        else if (IPtr[index - 2 * NX] == 12)
        {
            DWORD Color;
            if (i == LY-TY-1 && j == LX - 6)
            {
                if (j <= 0 || IPtr[index - 2 * NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (i == LY - TY - 1)
            {
                if (j <= 0 || IPtr[index - 2 * NX] == IPtr[index - 1]
                    || IPtr[index - NX + 5] == IPtr[index - 1] || IPtr[index - 2 * NX + 5] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX]
                        || Color == IPtr[index - NX + 5] || Color == IPtr[index - 2 * NX + 5])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (j == LX - 6)
            {
                if (j <= 0 || IPtr[index - 2 * NX] == IPtr[index - 1]
                    || IPtr[index + NX] == IPtr[index - 1] || IPtr[index + 1 + NX] == IPtr[index - 1]
                    || IPtr[index + 2 + NX] == IPtr[index - 1] || IPtr[index + 3 + NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX]
                        || Color == IPtr[index + NX] || Color == IPtr[index + 1 + NX]
                        || Color == IPtr[index + 2 + NX] || Color == IPtr[index + 3 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else
            {
                if (j <= 0 || IPtr[index - 2 * NX] == IPtr[index - 1]
                    || IPtr[index - NX + 5] == IPtr[index - 1] || IPtr[index - 2 * NX + 5] == IPtr[index - 1]
                    || IPtr[index + NX] == IPtr[index - 1] || IPtr[index + 1 + NX] == IPtr[index - 1]
                    || IPtr[index + 2 + NX] == IPtr[index - 1] || IPtr[index + 3 + NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX]
                        || Color == IPtr[index - NX + 5] || Color == IPtr[index - 2 * NX + 5]
                        || Color == IPtr[index + NX] || Color == IPtr[index + 1 + NX]
                        || Color == IPtr[index + 2 + NX] || Color == IPtr[index + 3 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_12(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_13(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 1] < 13)
                IPtr[index + 1] = (BYTE)Color;
            if (IPtr[index + 2] < 13)
                IPtr[index + 2] = (BYTE)Color;
            if (IPtr[index + 3] < 13)
                IPtr[index + 3] = (BYTE)Color;
            if (IPtr[index + 4 - NX] < 13)
                IPtr[index + 4 - NX] = (BYTE)Color;
            if (IPtr[index + 4 - 2 * NX] < 13)
                IPtr[index + 4 - 2 * NX] = (BYTE)Color;
            return 6;
        }
        else if (i <= 2)
            return 0;
        else
        {
            DWORD Color;
            if (i == LY-TY-1 && j == LX - 6)
            {
                if (j <= 0 || IPtr[index - 3 * NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (i == LY - TY - 1)
            {
                if (j <= 0 || IPtr[index - 3 * NX] == IPtr[index - 1]
                    || IPtr[index - NX + 5] == IPtr[index - 1] || IPtr[index - 2 * NX + 5] == IPtr[index - 1]
                    || IPtr[index - 3 * NX + 5] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 3 * NX]
                        || Color == IPtr[index - NX + 5] || Color == IPtr[index - 2 * NX + 5]
                        || Color == IPtr[index - 3 * NX + 5])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else if (j == LX - 6)
            {
                if (j <= 0 || IPtr[index - 3 * NX] == IPtr[index - 1]
                    || IPtr[index + NX] == IPtr[index - 1] || IPtr[index + 1 + NX] == IPtr[index - 1]
                    || IPtr[index + 2 + NX] == IPtr[index - 1] || IPtr[index + 3 + NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 3 * NX]
                        || Color == IPtr[index + NX] || Color == IPtr[index + 1 + NX]
                        || Color == IPtr[index + 2 + NX] || Color == IPtr[index + 3 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }
            else
            {
                if (j <= 0 || IPtr[index - 3 * NX] == IPtr[index - 1]
                    || IPtr[index - NX + 5] == IPtr[index - 1] || IPtr[index - 2 * NX + 5] == IPtr[index - 1]
                    || IPtr[index - 3 * NX + 5] == IPtr[index - 1]
                    || IPtr[index + NX] == IPtr[index - 1] || IPtr[index + 1 + NX] == IPtr[index - 1]
                    || IPtr[index + 2 + NX] == IPtr[index - 1] || IPtr[index + 3 + NX] == IPtr[index - 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 3 * NX]
                        || Color == IPtr[index - NX + 5] || Color == IPtr[index - 2 * NX + 5]
                        || Color == IPtr[index - 3 * NX + 5]
                        || Color == IPtr[index + NX] || Color == IPtr[index + 1 + NX]
                        || Color == IPtr[index + 2 + NX] || Color == IPtr[index + 3 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index - 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_12(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_13(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 1] < 13)
                IPtr[index + 1] = (BYTE)Color;
            if (IPtr[index + 2] < 13)
                IPtr[index + 2] = (BYTE)Color;
            if (IPtr[index + 3] < 13)
                IPtr[index + 3] = (BYTE)Color;
            if (IPtr[index + 4 - NX] < 13)
                IPtr[index + 4 - NX] = (BYTE)Color;
            if (IPtr[index + 4 - 2 * NX] < 13)
                IPtr[index + 4 - 2 * NX] = (BYTE)Color;
            if (IPtr[index + 4 - 3 * NX] < 13)
                IPtr[index + 4 - 3 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNITBA(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_18(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_17(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j >= LX-4 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i <= 0 || IPtr[index + 1] == IPtr[index - NX] || IPtr[index - 1] == IPtr[index - NX])
            {
                Color = 0;
                while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                    Color++;
            }
            else
                Color = IPtr[index - NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_17(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_18(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 4] == 12)
                return 4;
            else
                return 1;
        }
    }
    else if (BTemp == 14)
    {
        int index = i * NX + j;
        if (i <= 1 || i == LY - TY - 1)
            return 0;
        else if (IPtr[index - 2 * NX] == 12)
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_18(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 5;
        }
        else if (i <= 2)
            return 0;
        else
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_18(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        int index = i * NX + j;
        if (j >= LX-4 || i >= LY - TY - 2)
            return 0;
        else if (IPtr[index + 4] == 12)
        {
            DWORD Color;
            if (j == TX && i == LY - TY - 3)
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (j == TX)
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX]
                    || IPtr[index + 1 + 3 * NX] == IPtr[index - NX] || IPtr[index + 2 + 3 * NX] == IPtr[index - NX]
                    || IPtr[index + 3 + 3 * NX] == IPtr[index - NX] || IPtr[index + 4 + 3 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 + 3 * NX] || Color == IPtr[index + 2 + 3 * NX]
                        || Color == IPtr[index + 3 + 3 * NX] || Color == IPtr[index + 4 + 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (i == LY - TY - 3)
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX]
                    || IPtr[index - 1] == IPtr[index - NX] || IPtr[index - 1 + NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX]
                    || IPtr[index + 1 + 3 * NX] == IPtr[index - NX] || IPtr[index + 2 + 3 * NX] == IPtr[index - NX]
                    || IPtr[index + 3 + 3 * NX] == IPtr[index - NX] || IPtr[index + 4 + 3 * NX] == IPtr[index - NX]
                    || IPtr[index - 1] == IPtr[index - NX] || IPtr[index - 1 + NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 + 3 * NX] || Color == IPtr[index + 2 + 3 * NX]
                        || Color == IPtr[index + 3 + 3 * NX] || Color == IPtr[index + 4 + 3 * NX]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_17(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_18(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + NX] < 13)
                IPtr[index + NX] = (BYTE)Color;
            if (IPtr[index + 1 + 2 * NX] < 13)
                IPtr[index + 1 + 2 * NX] = (BYTE)Color;
            if (IPtr[index + 2 + 2 * NX] < 13)
                IPtr[index + 2 + 2 * NX] = (BYTE)Color;
            if (IPtr[index + 3 + 2 * NX] < 13)
                IPtr[index + 3 + 2 * NX] = (BYTE)Color;
            if (IPtr[index + 4 + 2 * NX] < 13)
                IPtr[index + 4 + 2 * NX] = (BYTE)Color;
            return 6;
        }
        else if (i >= LY - TY - 3)
            return 0;
        else
        {
            DWORD Color;
            if (j == TX && i == LY - TY - 4)
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (j == TX)
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX]
                    || IPtr[index + 1 + 4 * NX] == IPtr[index - NX] || IPtr[index + 2 + 4 * NX] == IPtr[index - NX]
                    || IPtr[index + 3 + 4 * NX] == IPtr[index - NX] || IPtr[index + 4 + 4 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 + 4 * NX] || Color == IPtr[index + 2 + 4 * NX]
                        || Color == IPtr[index + 3 + 4 * NX] || Color == IPtr[index + 4 + 4 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else if (i == LY - TY - 4)
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX]
                    || IPtr[index - 1] == IPtr[index - NX] || IPtr[index - 1 + NX] == IPtr[index - NX]
                    || IPtr[index - 1 + 2 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 + NX]
                        || Color == IPtr[index - 1 + 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }
            else
            {
                if (i <= 0 || IPtr[index + 4] == IPtr[index - NX]
                    || IPtr[index + 1 + 4 * NX] == IPtr[index - NX] || IPtr[index + 2 + 4 * NX] == IPtr[index - NX]
                    || IPtr[index + 3 + 4 * NX] == IPtr[index - NX] || IPtr[index + 4 + 4 * NX] == IPtr[index - NX]
                    || IPtr[index - 1] == IPtr[index - NX] || IPtr[index - 1 + NX] == IPtr[index - NX]
                    || IPtr[index - 1 + 2 * NX] == IPtr[index - NX])
                {
                    Color = 0;
                    while (Color == IPtr[index + 4]
                        || Color == IPtr[index + 1 + 4 * NX] || Color == IPtr[index + 2 + 4 * NX]
                        || Color == IPtr[index + 3 + 4 * NX] || Color == IPtr[index + 4 + 4 * NX]
                        || Color == IPtr[index - 1] || Color == IPtr[index - 1 + NX]
                        || Color == IPtr[index - 1 + 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index - NX];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_17(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_18(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + NX] < 13)
                IPtr[index + NX] = (BYTE)Color;
            if (IPtr[index + 2 * NX] < 13)
                IPtr[index + 2 * NX] = (BYTE)Color;
            if (IPtr[index + 1 + 3 * NX] < 13)
                IPtr[index + 1 + 3 * NX] = (BYTE)Color;
            if (IPtr[index + 2 + 3 * NX] < 13)
                IPtr[index + 2 + 3 * NX] = (BYTE)Color;
            if (IPtr[index + 3 + 3 * NX] < 13)
                IPtr[index + 3 + 3 * NX] = (BYTE)Color;
            if (IPtr[index + 4 + 3 * NX] < 13)
                IPtr[index + 4 + 3 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

BYTE LCreateUNITBB(FILE* fin, BYTE* IPtr, DWORD DIM, int LX, int LY, int TX, int TY, int i, int j, DWORD MY, int* CCount, DWORD Player)
{
    BYTE BTemp = IPtr[i * NX + j];
    if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Minernal / 13 ~ 15 : Height / Width / Corner Black
    {
        CreateUNIT(fin, MakeXY_18(j, i, MY), BTemp, 0xD6);
        (*CCount)++;
    }
    else if (BTemp == 12)
    {
        CreateUNIT(fin, MakeXY_17(j, i, MY), Player, 0xBC);
        (*CCount)++;
    }
    else if (BTemp == 13)
    {
        if (j >= LX-4 || j == TX)
            return 0;
        else
        {
            DWORD Color;
            int index = i * NX + j;
            if (i <= 0 || IPtr[index + 1] == IPtr[index - NX] || IPtr[index - 1] == IPtr[index - NX])
            {
                Color = 0;
                while (Color == IPtr[index + 1] || Color == IPtr[index - 1])
                    Color++;
            }
            else
                Color = IPtr[index - NX];
            if (Color == 12)
                CreateUNIT(fin, MakeXY_17(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_18(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index + 4] == 12)
                return 4;
            else
                return 1;
        }
    }
    else if (BTemp == 14)
    {
        int index = i * NX + j;
        if (i <= 1 || i == LY - TY - 1)
            return 0;
        else if (IPtr[index - 2 * NX] == 12)
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_18(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 5;
        }
        else if (i <= 2)
            return 0;
        else
        {
            DWORD Color;
            Color = 0;
            while (Color == IPtr[index + NX] || Color == IPtr[index - NX])
                Color++;
            CreateUNIT(fin, MakeXY_18(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            return 2;
        }
    }
    else if (BTemp == 15)
    {
        int index = i * NX + j;
        if (i <= 1 || j <= TX-3)
            return 0;
        else if (IPtr[index - 2 * NX] == 12)
        {
            DWORD Color;
            if (i == LY - TY - 1 && j == TX-4)
            {
                if (j >= LX-1 || IPtr[index - 2 * NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (i == 1)
            {
                if (j >= LX - 1 || IPtr[index - 2 * NX] == IPtr[index + 1]
                    || IPtr[index - NX - 5] == IPtr[index + 1] || IPtr[index - 2 * NX - 5] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX]
                        || Color == IPtr[index - NX - 5] || Color == IPtr[index - 2 * NX - 5])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (j == TX-4)
            {
                if (j >= LX - 1 || IPtr[index - 2 * NX] == IPtr[index + 1]
                    || IPtr[index + NX] == IPtr[index + 1] || IPtr[index - 1 + NX] == IPtr[index + 1]
                    || IPtr[index - 2 + NX] == IPtr[index + 1] || IPtr[index - 3 + NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX]
                        || Color == IPtr[index + NX] || Color == IPtr[index - 1 + NX]
                        || Color == IPtr[index - 2 + NX] || Color == IPtr[index - 3 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else
            {
                if (j >= LX - 1 || IPtr[index - 2 * NX] == IPtr[index + 1]
                    || IPtr[index - NX - 5] == IPtr[index + 1] || IPtr[index - 2 * NX - 5] == IPtr[index + 1]
                    || IPtr[index + NX] == IPtr[index + 1] || IPtr[index - 1 + NX] == IPtr[index + 1]
                    || IPtr[index - 2 + NX] == IPtr[index + 1] || IPtr[index - 3 + NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 2 * NX]
                        || Color == IPtr[index - NX - 5] || Color == IPtr[index - 2 * NX - 5]
                        || Color == IPtr[index + NX] || Color == IPtr[index - 1 + NX]
                        || Color == IPtr[index - 2 + NX] || Color == IPtr[index - 3 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_17(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_18(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            
            if (IPtr[index - 1] < 13)
                IPtr[index - 1] = (BYTE)Color;
            if (IPtr[index - 2] < 13)
                IPtr[index - 2] = (BYTE)Color;
            if (IPtr[index - 3] < 13)
                IPtr[index - 3] = (BYTE)Color;
            if (IPtr[index - 4 - NX] < 13)
                IPtr[index - 4 - NX] = (BYTE)Color;
            if (IPtr[index - 4 - 2 * NX] < 13)
                IPtr[index - 4 - 2 * NX] = (BYTE)Color;
           
            return 6;
        }
        else if (i <= 2)
            return 0;
        else
        {
            DWORD Color;
            if (i == LY - TY - 1 && j == TX-4)
            {
                if (j >= LX-1 || IPtr[index - 3 * NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 3 * NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (i == LY - TY - 1)
            {
                if (j >= LX - 1 || IPtr[index - 3 * NX] == IPtr[index + 1]
                    || IPtr[index - NX - 5] == IPtr[index + 1] || IPtr[index - 2 * NX - 5] == IPtr[index + 1]
                    || IPtr[index - 3 * NX - 5] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 3 * NX]
                        || Color == IPtr[index - NX - 5] || Color == IPtr[index - 2 * NX - 5]
                        || Color == IPtr[index - 3 * NX - 5])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else if (j == TX-4)
            {
                if (j >= LX - 1 || IPtr[index - 3 * NX] == IPtr[index + 1]
                    || IPtr[index + NX] == IPtr[index + 1] || IPtr[index - 1 + NX] == IPtr[index + 1]
                    || IPtr[index - 2 + NX] == IPtr[index + 1] || IPtr[index - 3 + NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 3 * NX]
                        || Color == IPtr[index + NX] || Color == IPtr[index - 1 + NX]
                        || Color == IPtr[index - 2 + NX] || Color == IPtr[index - 3 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }
            else
            {
                if (j >= LX - 1 || IPtr[index - 3 * NX] == IPtr[index + 1]
                    || IPtr[index - NX - 5] == IPtr[index + 1] || IPtr[index - 2 * NX - 5] == IPtr[index + 1]
                    || IPtr[index - 3 * NX - 5] == IPtr[index + 1]
                    || IPtr[index + NX] == IPtr[index + 1] || IPtr[index - 1 + NX] == IPtr[index + 1]
                    || IPtr[index - 2 + NX] == IPtr[index + 1] || IPtr[index - 3 + NX] == IPtr[index + 1])
                {
                    Color = 0;
                    while (Color == IPtr[index - 3 * NX]
                        || Color == IPtr[index - NX - 5] || Color == IPtr[index - 2 * NX - 5]
                        || Color == IPtr[index - 3 * NX - 5]
                        || Color == IPtr[index + NX] || Color == IPtr[index - 1 + NX]
                        || Color == IPtr[index - 2 + NX] || Color == IPtr[index - 3 + NX])
                        Color++;
                }
                else
                    Color = IPtr[index + 1];
            }

            if (Color == 12)
                CreateUNIT(fin, MakeXY_17(j, i, MY), Player, 0xBC);
            else
                CreateUNIT(fin, MakeXY_18(j, i, MY), Color, 0xD6);
            (*CCount)++;
            IPtr[index] = (BYTE)Color;
            if (IPtr[index - 1] < 13)
                IPtr[index - 1] = (BYTE)Color;
            if (IPtr[index - 2] < 13)
                IPtr[index - 2] = (BYTE)Color;
            if (IPtr[index - 3] < 13)
                IPtr[index - 3] = (BYTE)Color;
            if (IPtr[index - 4 - NX] < 13)
                IPtr[index - 4 - NX] = (BYTE)Color;
            if (IPtr[index - 4 - 2 * NX] < 13)
                IPtr[index - 4 - 2 * NX] = (BYTE)Color;
            if (IPtr[index - 4 - 3 * NX] < 13)
                IPtr[index - 4 - 3 * NX] = (BYTE)Color;
            return 3;
        }
    }
    return 0;
}

DWORD PatchUNIT(FILE *fchk,FILE* fin)
{
    /*
       UNIT Section
       DWORD Local ID = 0x00000000
       WORD X(+0x4) / WORD Y(+0x6)
       WORD UnitId(+0x8) (StartLoc 0xD6, Mineral 0xB0 0xB1 0xB2)
       BYTE PlayerId(+0x10)
       00000000 XXXXYYYY UNIT0000 18000300 Pi646464 00000000 00000000 00000000 00000000
    */
    DWORD Size, Temp;
    fseek(fin, 0, 2);
    Size = ftell(fin);
    fseek(fin, 0x8, 0);
    
    DWORD Buffer[9] = { 0 };
    BYTE* Bptr = (BYTE*)Buffer;
    // Load Map Size
    DWORD DIM = 0;
    GetChkSection(fchk, "DIM ");
    fseek(fchk, Ret1+8, 0);
    fread(&DIM, 4, 1, fchk);
    WORD MX = DIM % 65536, MY = DIM / 65536;
    DIM = (MX >= MY) ? MX : MY;
    printf("맵 크기 %d x %d (미니맵 크기 : %d)\n", MX, MY, DIM);
    if (DIM != 64 && DIM != 96 && DIM != 128 && DIM != 192 && DIM != 256)
    {   
        printf("비정상 맵 크기가 감지되었습니다. 프로그램을 종료합니다\n");
        return -1;
    }

    // Load StartLocation
    DWORD Starting[8] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF };
    for (DWORD i = 8; i < Size; i += 0x24)
    {   
        fseek(fin, i, 0);
        fread(Buffer, 0x24, 1, fin);

        if (Bptr[8] == 0xD6 && Bptr[16] < 8)
        {
            Starting[Bptr[16]] = Buffer[1];
            printf("P%d의 스타트 로케이션 : X = %d, Y = %d 로드 완료\n",Bptr[16]+1,Buffer[1]%0x10000, Buffer[1]/0x10000);
        }
    }
    
    DWORD Mode;
    Retry3:
    printf("\n썸네일 드로잉 모드를 선택하세요 (1 또는 2 입력)\n1. 고화질 중첩 방식 (화질 2배, 배경색 불가, 16색 팔레트)\n2. 저화질 배치 방식 (화질 1/0.5배, 배경색 가능, 단색/12색 팔레트)\n");
    scanf_s("%d", &Mode);
    if (Mode != 1 && Mode != 2)
        goto Retry3;

    DWORD P16[2] = { 0 };
    int LCount = 0;
    while (true)
    {
        int CCount = 0;
        Input:
        char image[512] = { 0 };
        printf("\n삽입할 이미지 파일이름을 입력하세요 (0 입력시 종료)\n");
        scanf_s("%s", image, 512);

        if (!strcmp(image,"0"))
            break;
        else
        {
            FILE* fnew;
            fopen_s(&fnew, image, "rb");
            if (fnew == NULL)
            {
                printf("%s 로드 실패\n", image);
                goto Input;
            }
            else
            {
                BYTE Color;
                BITMAP BMP;
                HBITMAP HBMP = (HBITMAP)LoadImageA(0, image, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
                GetObject(HBMP, sizeof(BMP), (LPSTR)&BMP);
                BYTE* IPtr = (BYTE*)BMP.bmBits;
                DWORD X = BMP.bmWidth, Y = BMP.bmHeight;
                if (X % 4 == 0)
                    NX = X;
                else if (X % 4 == 1)
                    NX = X + 3;
                else if (X % 4 == 2)
                    NX = X + 2;
                else if (X % 4 == 3)
                    NX = X + 1;

                fseek(fnew, 0x2E, 0);
                fread(&Color, 1, 1, fnew);
                if (Mode == 2)
                {
                    if (Color == 2)
                    {
                        printf("%s 로드 완료, 크기 %d x %d (단색 팔레트)\n\n", image, X, Y);
                        if (DIM == 64)
                        {
                            if (X != (MX >> 1) || Y != (MY >> 1))
                            {
                                printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX >> 1, MY >> 1);
                                goto BEND;
                            }
                        }
                        else if (DIM == 128 || DIM == 96)
                        {
                            if (X != (MX >> 1) || Y != (MY >> 1))
                            {
                                printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX >> 1, MY >> 1);
                                goto BEND;
                            }
                        }
                        else if (DIM == 192 || DIM == 256)
                        {
                            if (X != (MX >> 2) || Y != (MY >> 2))
                            {
                                printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX >> 2, MY >> 2);
                                goto BEND;
                            }
                        }

                        if (P16[0] == 0)
                        {
                        Retry:
                            printf("이미지를 그리는데 사용할 미네랄 필드의 타입을 입력하세요 (1,2,3중 선택) : ");
                            scanf_s("%u", &P16[0]);
                            if (P16[0] != 1 && P16[0] != 2 && P16[0] != 3)
                                goto Retry;

                        Retry2:
                            printf("사용할 미네랄 필드의 플레이어를 입력하세요 (1~12중 선택) : ");
                            scanf_s("%u", &P16[1]);
                            if (P16[1] < 1 || P16[1] > 12)
                                goto Retry2;

                            printf("\nP%d의 미네랄 필드 (타입%d)으로 이미지를 그립니다 (시작후 해당유닛 RemoveUnit액션 필요)\n\n", P16[1], P16[0]);
                            P16[1]--;
                        }

                        if (DIM == 64)
                        {
                            printf("이미지의 X좌표 위치 편차를 입력해주세요 (-3~+3)중 선택\n");
                            scanf_s("%d", &ModeX);
                            printf("이미지의 Y좌표 위치 편차를 입력해주세요 (-3~+3)중 선택\n");
                            scanf_s("%d", &ModeY);
                            ModeX = ModeX * 16;
                            ModeY = ModeY * 16;
                        }
                        else if (DIM == 128 || DIM == 96)
                        {
                            printf("이미지의 X좌표 위치 편차를 입력해주세요 (-1~+1)중 선택\n");
                            scanf_s("%d", &ModeX);
                            printf("이미지의 Y좌표 위치 편차를 입력해주세요 (-1~+1)중 선택\n");
                            scanf_s("%d", &ModeY);
                            ModeX = ModeX * 32;
                            ModeY = ModeY * 32;
                        }
                        else if (DIM == 192 || DIM == 256)
                        {
                            printf("이미지의 X좌표 위치 편차를 입력해주세요 (-1~+1)중 선택\n");
                            scanf_s("%d", &ModeX);
                            printf("이미지의 Y좌표 위치 편차를 입력해주세요 (-1~+1)중 선택\n");
                            scanf_s("%d", &ModeY);
                            ModeX = ModeX * 64;
                            ModeY = ModeY * 64;
                        }

                        fseek(fin, 0, 2);
                        BYTE* IPtr = (BYTE*)BMP.bmBits;
                        int LX, LY;
                        if (DIM == 64)
                            LX = MX / 2, LY = MY / 2;
                        else if (DIM == 96 || DIM == 128)
                            LX = MX / 2, LY = MY / 2;
                        else if (DIM == 192 || DIM == 256)
                            LX = MX / 4, LY = MY / 4;
                        for (int i = 0; i < LY; i++)
                        {
                            for (int j = 0; j < LX; j++)
                            {
                                if (IPtr[i * NX + j] == 0) // 0 : Draw, 1 : Blank
                                {
                                    if (DIM == 64)
                                    {
                                        CreateUNIT(fin, MakeXY2(0, j, i, MY), P16[1], P16[0]);
                                        CCount++;
                                        CreateUNIT(fin, MakeXY2(1, j, i, MY), P16[1], P16[0]);
                                        CCount++;
                                    }
                                    else
                                    {
                                        CreateUNIT(fin, MakeXY1(DIM, j, i, MY), P16[1], P16[0]);
                                        CCount++;
                                    }
                                }
                            }
                        }
                        printf("Layer%d : %d개의 유닛이 추가되었습니다\n", LCount++, CCount);
                    }
                    else
                    {
                        printf("%s 로드 완료, 크기 %d x %d (12색 팔레트)\n\n", image, X, Y);
                        if (DIM == 64)
                        {
                            if (X != (MX >> 1) || Y != (MY >> 1))
                            {
                                printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX >> 1, MY >> 1);
                                goto BEND;
                            }
                        }
                        else if (DIM == 128 || DIM == 96)
                        {
                            if (X != (MX >> 2) || Y != (MY >> 2))
                            {
                                printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX >> 1, MY >> 1);
                                goto BEND;
                            }
                        }
                        else if (DIM == 192 || DIM == 256)
                        {
                            if (X != (MX >> 2) || Y != (MY >> 2))
                            {
                                printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX >> 2, MY >> 2);
                                goto BEND;
                            }
                        }

                        if (DIM == 64)
                        {
                            printf("이미지의 X좌표 위치 편차를 입력해주세요 (-3~+3)중 선택\n");
                            scanf_s("%d", &ModeX);
                            printf("이미지의 Y좌표 위치 편차를 입력해주세요 (-3~+3)중 선택\n");
                            scanf_s("%d", &ModeY);
                            ModeX = ModeX * 16;
                            ModeY = ModeY * 16;
                        }
                        else if (DIM == 128 || DIM == 96)
                        {
                            printf("이미지의 X좌표 위치 편차를 입력해주세요 (-3~+3)중 선택\n");
                            scanf_s("%d", &ModeX);
                            printf("이미지의 Y좌표 위치 편차를 입력해주세요 (-3~+3)중 선택\n");
                            scanf_s("%d", &ModeY);
                            ModeX = ModeX * 32;
                            ModeY = ModeY * 32;
                        }
                        else if (DIM == 192 || DIM == 256)
                        {
                            printf("이미지의 X좌표 위치 편차를 입력해주세요 (-1~+1)중 선택\n");
                            scanf_s("%d", &ModeX);
                            printf("이미지의 Y좌표 위치 편차를 입력해주세요 (-1~+1)중 선택\n");
                            scanf_s("%d", &ModeY);
                            ModeX = ModeX * 64;
                            ModeY = ModeY * 64;
                        }

                        fseek(fin, 0, 2);
                        BYTE* IPtr = (BYTE*)BMP.bmBits;
                        int LX, LY;
                        if (DIM == 64)
                            LX = MX / 2, LY = MY / 2;
                        else if (DIM == 96 || DIM == 128)
                            LX = MX / 4, LY = MY / 4;
                        else if (DIM == 192 || DIM == 256)
                            LX = MX / 4, LY = MY / 4;
                        for (int i = 0; i < LY; i++)
                        {
                            for (int j = 0; j < LX; j++)
                            {
                                BYTE BTemp = IPtr[i * NX + j];
                                if (BTemp < 12) // 0 ~ 11 : Player Color / 12 : Blank
                                {
                                    if (DIM == 64)
                                    {
                                        CreateUNIT(fin, MakeXY3(j, i, MY), BTemp, 0xD6);
                                        CCount++;
                                    }
                                    else if (DIM == 96 || DIM == 128)
                                    {
                                        CreateUNIT(fin, MakeXY4(DIM, 0, j, i, MY), BTemp, 0xD6);
                                        CCount++;
                                        CreateUNIT(fin, MakeXY4(DIM, 1, j, i, MY), BTemp, 0xD6);
                                        CCount++;
                                    }
                                    else
                                    {
                                        CreateUNIT(fin, MakeXY1(DIM, j, i, MY), BTemp, 0xD6);
                                        CCount++;
                                    }
                                }
                            }
                        }
                        printf("Layer%d : %d개의 유닛이 추가되었습니다\n", LCount++, CCount);

                        printf("스타팅을 복구하려면 0을 입력하세요 (미 복구시 시작후 CenterView액션 필요) : ");
                        int Recover, RCount = 0;
                        scanf_s("%d", &Recover);
                        if (Recover == 0)
                        {
                            for (int l = 0; l < 8; l++)
                            {
                                if (Starting[l] != 0xFFFFFFFF)
                                {
                                    CreateUNIT(fin, Starting[l], l, 0xD6);
                                    RCount++;
                                }
                            }
                            printf("각 플레이어의 스타팅이 복구되었습니다 (Add %d Units)\n", RCount);
                        }
                    }
                }
                else if (Mode == 1)
                {
                    if (DIM == 64)
                    {
                        printf("%s 로드 완료, 크기 %d x %d (16색 팔레트)\n\n", image, X, Y);
                        if (X != (MX*2) || Y != (MY*2)) // 2n-1
                        {
                            printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX * 2, MY * 2);
                            goto BEND;
                        }
                    }
                    else if (DIM == 128 || DIM == 96)
                    {
                        printf("%s 로드 완료, 크기 %d x %d (16색 팔레트)\n\n", image, X, Y);
                        if (X != MX || Y != MY) // n-1
                        {
                            printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX , MY );
                            goto BEND;
                        }
                    }
                    else if (DIM == 192 || DIM == 256)
                    {
                        printf("%s 로드 완료, 크기 %d x %d (16색 팔레트)\n\n", image, X, Y);
                        if (X != (MX/2) || Y != (MY/2)) // Math.floor(n/2)-1
                        {
                            printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX/2, MY/2);
                            goto BEND;
                        }
                    }

                    if (P16[0] == 0)
                    {
                        P16[0] = 13;
                        Retry_2:
                        printf("사용할 베스핀 가스의 플레이어를 입력하세요 (1~12중 선택) : ");
                        scanf_s("%u", &P16[1]);
                        if (P16[1] < 1 || P16[1] > 12)
                            goto Retry_2;

                        printf("\nP%d의 베스핀 가스로 이미지를 그립니다 (시작후 해당유닛 RemoveUnit액션 필요)\n\n", P16[1]);
                        P16[1]--;
                    }

                    fseek(fin, 0, 2);
                    BYTE* IPtr = (BYTE*)BMP.bmBits;
                    int LX, LY;
                    if (DIM == 64)
                        LX = MX*2, LY = MY*2;
                    else if (DIM == 96 || DIM == 128)
                        LX = MX, LY = MY;
                    else if (DIM == 192 || DIM == 256)
                        LX = MX/2, LY = MY/2;
                    
                    DWORD MDir;
                Retry_1:
                    if (DIM == 256 || DIM == 192)
                        printf("이미지가 잘리는 모서리의 방향을 설정하세요\n1. 오른쪽 아래 방향 (↘) [권장, 실제 화질 : %dx%d]\n2. 왼쪽 아래 방향 (↙) [실제 화질 : %dx%d]\n3. 오른쪽 위 방향 (↗) [실제 화질 : %dx%d]\n4. 왼쪽 위 방향 (↖) [실제 화질 : %dx%d]\n", LX - 1, LY - 1, LX - 1, LY - 1, LX - 1, LY - 1, LX - 1, LY - 1);
                    else if (DIM == 128 || DIM == 96)
                        printf("이미지가 잘리는 모서리의 방향을 설정하세요\n1. 오른쪽 아래 방향 (↘) [권장, 실제 화질 : %dx%d]\n2. 왼쪽 아래 방향 (↙) [실제 화질 : %dx%d]\n3. 오른쪽 위 방향 (↗) [실제 화질 : %dx%d]\n4. 왼쪽 위 방향 (↖) [실제 화질 : %dx%d]\n", LX - 1, LY - 1, LX - 3, LY - 1, LX - 1, LY - 2, LX - 3, LY - 2);
                    else if (DIM == 64)
                        printf("이미지가 잘리는 모서리의 방향을 설정하세요\n1. 오른쪽 아래 방향 (↘) [권장, 실제 화질 : %dx%d]\n2. 왼쪽 아래 방향 (↙) [실제 화질 : %dx%d]\n3. 오른쪽 위 방향 (↗) [실제 화질 : %dx%d]\n4. 왼쪽 위 방향 (↖) [실제 화질 : %dx%d]\n", LX - 1, LY - 1, LX - 3, LY - 1, LX - 1, LY - 3, LX - 3, LY - 3);

                    scanf_s("%d", &MDir);
                    if (MDir != 1 && MDir != 2 && MDir != 3 && MDir != 4)
                        goto Retry_1;

                    DWORD Dir;
                    if (MDir == 1)
                    {
                        Retry_3:
                        printf("중첩시 생성되는 그림자의 방향을 설정하세요\n1. 오른쪽 방향 (→)\n2. 아래쪽 방향 (↓)\n");
                        scanf_s("%d", &Dir);
                        if (Dir != 1 && Dir != 2)
                            goto Retry_3;
                    }
                    else if (MDir == 2)
                    {
                        Retry_4:
                        printf("중첩시 생성되는 그림자의 방향을 설정하세요\n1. 왼쪽 방향 (←)\n2. 아래쪽 방향 (↓)\n");
                        scanf_s("%d", &Dir);
                        if (Dir != 1 && Dir != 2)
                            goto Retry_4;
                        Dir += 2;
                    }
                    else if (MDir == 3)
                    {
                    Retry_5:
                        printf("중첩시 생성되는 그림자의 방향을 설정하세요\n1. 오른쪽 방향 (→)\n2. 위쪽 방향 (↑)\n");
                        scanf_s("%d", &Dir);
                        if (Dir != 1 && Dir != 2)
                            goto Retry_5;
                        Dir += 4;
                    }
                    else if (MDir == 4)
                    {
                    Retry_6:
                        printf("중첩시 생성되는 그림자의 방향을 설정하세요\n1. 왼쪽 방향 (←)\n2. 위쪽 방향 (↑)\n");
                        scanf_s("%d", &Dir);
                        if (Dir != 1 && Dir != 2)
                            goto Retry_6;
                        Dir += 6;
                    }

                    Retry_X:
                    printf("그림자 제거 옵션을 선택하세요\n1. 그림자 완전 제거 (경우에 따라 일부 그림자는 남을수도 있음)\n2. 그림자 마스크 적용 (단색 팔레트 이미지 사용)\n3. 그림자 제거 안함\n");
                    DWORD Clear;
                    scanf_s("%d", &Clear);
                    if (Clear != 1 && Clear != 2 && Clear != 3)
                        goto Retry_X;
                    BYTE MaskFlag[128 * 128] = { 0 };
                    if (Clear == 1)
                        for (int p = 0; p < 16384; p++)
                            MaskFlag[p] = 1;
                    if (Clear == 2)
                    {
                    Retry_Z:
                        char Mask[512] = { 0 };
                        printf("\n삽입할 이미지 파일이름을 입력하세요 (0 입력시 취소)\n"); // 0 = 제거X (검정), 1 = 제거(하양)
                        scanf_s("%s", Mask, 512);

                        if (!strcmp(Mask, "0"))
                        {
                            Clear = 3;
                            goto Retry_Y;
                        }
                        else
                        {
                            FILE* fMask;
                            fopen_s(&fMask, Mask, "rb");
                            if (fMask == NULL)
                            {
                                printf("%s 로드 실패\n", Mask);
                                goto Retry_Z;
                            }
                            else
                            {
                                int FX, FY;
                                fseek(fMask, 18, 0);
                                fread(&FX, 4, 1, fMask);
                                fseek(fMask, 22, 0);
                                fread(&FY, 4, 1, fMask);
                                printf("%s 로드 완료, 크기 %d x %d (단색 팔레트)\n\n", Mask, FX, FY);
                                if (DIM == 64)
                                {
                                    if (FX != (MX * 2) || FY != (MY * 2)) // 2n-1
                                    {
                                        printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX * 2, MY * 2);
                                        fclose(fMask);
                                        goto Retry_Z;
                                    }
                                }
                                else if (DIM == 128 || DIM == 96)
                                {
                                    if (FX != MX || FY != MY) // n-1
                                    {
                                        printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX, MY);
                                        fclose(fMask);
                                        goto Retry_Z;
                                    }
                                }
                                else if (DIM == 192 || DIM == 256)
                                {
                                    if (FX != (MX / 2) || FY != (MY / 2)) // Math.floor(n/2)-1
                                    {
                                        printf("이미지의 크기가 %d x %d가 아닙니다. 다른 이미지를 선택해주세요\n", MX / 2, MY / 2);
                                        fclose(fMask);
                                        goto Retry_Z;
                                    }
                                }
                                for (int u = FY-1; u >= 0; u--)
                                {
                                    for (int v = 0; v < FX; v++)
                                    {
                                        int index = u * NX + v;
                                        BYTE Temp;
                                        fseek(fMask, index + 0x3E, 0);
                                        fread(&Temp, 1, 1, fMask);
                                        MaskFlag[index] = Temp;
                                    }
                                }
                                fclose(fMask);
                            }
                        }
                    }
                    Retry_Y:

                    BYTE FieldFlag[128 * 128] = { 0 };
                    BYTE Flag = 0;
                    
                    int OCount = 0, PCount[2] = { 0 }, QCount = 0;
                    if (Dir == 1) // →↓ X = 0~126, Y = 1~127 (↗)
                    {
                        for (int i = LY - 1; i > 0; i--)
                        {
                            for (int j = 0; j < LX - 1; j++) 
                            {
                                int Index = i * NX + j;
                                if (DIM == 256 || DIM == 192)
                                {
                                    Flag = LCreateUNIT1A(fin, IPtr, DIM, LX, LY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index - 1 - 2 * NX] = 3;
                                    if (FieldFlag[Index] == 3)
                                    {
                                        CreateUNITX(fin, MakeXY_1(j - 1, i + 2, MY), IPtr[Index - 1 + 2 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] == 2)
                                        {
                                            CreateUNITX(fin, MakeXY_1(j - 1, i + 2, MY), IPtr[Index - 1 + 2 * NX], P16[1]);
                                            FieldFlag[Index - 1] = 0;
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 2;
                                        else
                                            CreateUNITX(fin, MakeXY_1(j, i + 2, MY), IPtr[Index + 2 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] == 1)
                                        {
                                            CreateUNITX(fin, MakeXY_1(j - 2, i + 1, MY), IPtr[Index - 2 + NX], P16[1]);
                                            FieldFlag[Index + NX] = 0;
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_1(j - 2, i, MY), IPtr[Index - 2], P16[1]);
                                    }
                                }
                                else if (DIM == 64)
                                {
                                    Flag = LCreateUNIT5A(fin, IPtr, DIM, LX, LY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index - 1 - 4 * NX] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX1(fin, j - 3, i + 4, MY, IPtr[Index - 3 + 4 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 4)
                                        {  
                                            if (j <= LX - 3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] >= 4 && FieldFlag[Index - 1] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 4)
                                            {
                                                CreateUNITX1(fin, j - 1, i + 4, MY, IPtr[Index - 1 + 4 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 5)
                                            {
                                                CreateUNITX1(fin, j - 2, i + 4, MY, IPtr[Index - 2 + 4 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 6)
                                            {
                                                CreateUNITX1(fin, j - 3, i + 4, MY, IPtr[Index - 3 + 4 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX1(fin, j, i + 4, MY, IPtr[Index + 4 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] >= 1)
                                        {
                                            if (i >= 2 && IPtr[Index - NX] == 13 && FieldFlag[Index + NX] <= 2)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + NX];
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 1)
                                            {
                                                CreateUNITX1(fin, j - 4, i + 1, MY, IPtr[Index - 4 + NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 2)
                                            {
                                                CreateUNITX1(fin, j - 4, i + 2, MY, IPtr[Index - 4 + 2*NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 3)
                                            {
                                                CreateUNITX1(fin, j - 4, i + 3, MY, IPtr[Index - 4 + 3*NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX1(fin, j - 4, i, MY, IPtr[Index - 4], P16[1]);
                                    }
                                }
                                else
                                {
                                    Flag = LCreateUNIT9A(fin, IPtr, DIM, LX, LY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index - 1 - 3 * NX] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX(fin, MakeXY_3(j - 3, i + 3, MY), IPtr[Index - 3 + 3 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 6)
                                        FieldFlag[Index - 1 - 2 * NX] = 8;
                                    if (FieldFlag[Index] == 8)
                                    {
                                        CreateUNITX(fin, MakeXY_2(j - 3, i + 2, MY), IPtr[Index - 3 + 2 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 4 && FieldFlag[Index - 1] <= 6)
                                        {
                                            if (j <= LX - 3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] >= 4 && FieldFlag[Index - 1] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 4)
                                            {
                                                CreateUNITX(fin, MakeXY_3(j - 1, i + 3, MY), IPtr[Index - 1 + 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 5)
                                            {
                                                CreateUNITX(fin, MakeXY_3(j - 2, i + 3, MY), IPtr[Index - 2 + 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 6)
                                            {
                                                CreateUNITX(fin, MakeXY_3(j - 3, i + 3, MY), IPtr[Index - 3 + 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX(fin, MakeXY_3(j, i + 3, MY), IPtr[Index + 3 * NX], P16[1]);
                                    }
                                    if (Flag == 5)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 9 && FieldFlag[Index - 1] <= 11)
                                        {
                                            if (j <= LX - 3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] >= 9 && FieldFlag[Index - 1] <= 10)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 9)
                                            {
                                                CreateUNITX(fin, MakeXY_2(j - 1, i + 2, MY), IPtr[Index - 1 + 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 10)
                                            {
                                                CreateUNITX(fin, MakeXY_2(j - 2, i + 2, MY), IPtr[Index - 2 + 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 11)
                                            {
                                                CreateUNITX(fin, MakeXY_2(j - 3, i + 2, MY), IPtr[Index - 3 + 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 9;
                                        else
                                            CreateUNITX(fin, MakeXY_2(j, i + 2, MY), IPtr[Index + 2 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] >= 1 && FieldFlag[Index + NX] <= 2)
                                        {
                                            if (i >= 2 && IPtr[Index - NX] == 13 && FieldFlag[Index + NX] <= 1)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + NX];
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 1)
                                            {
                                                CreateUNITX(fin, MakeXY_3(j - 4, i + 1, MY), IPtr[Index - 4 + NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 2)
                                            {
                                                CreateUNITX(fin, MakeXY_3(j - 4, i + 2, MY), IPtr[Index - 4 + 2 * NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_3(j - 4, i, MY), IPtr[Index - 4], P16[1]);
                                    }
                                    if (Flag == 4)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] >= 1 && FieldFlag[Index + NX] <= 2)
                                        {
                                            if (FieldFlag[Index + NX] == 1)
                                                CreateUNITX(fin, MakeXY_2(j - 4, i + 1, MY), IPtr[Index - 4 + NX], P16[1]);
                                            else
                                                CreateUNITX(fin, MakeXY_2(j - 4, i + 2, MY), IPtr[Index - 4 + 2*NX], P16[1]);
                                            FieldFlag[Index + NX] = 0;
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_2(j - 4, i, MY), IPtr[Index - 4], P16[1]);
                                    }
                                }
                                if (MaskFlag[Index] == 1)
                                {
                                    if (i >= LY - 1)
                                        continue;
                                    if (DIM == 256 || DIM == 192)
                                    {
                                        if (IPtr[Index] != IPtr[Index + 1 + NX])
                                        {
                                            if (j >= LX - 2)
                                                continue;
                                            int Type = 0;
                                            if (j == LX - 3)
                                                Type = 2;
                                            else if (j == LX - 4)
                                                Type = 1;

                                            if (IPtr[Index + 1 + NX] == IPtr[Index + 1])
                                                OCount++;
                                            if (Check1(IPtr, NX, Index, Type) == 1)
                                            {
                                                if (IPtr[Index + 1 + NX] < 12)
                                                    CreateUNIT(fin, MakeXY_1(j + 1, i + 1, MY), IPtr[Index + 1 + NX], 0xD6);
                                                else if (IPtr[Index + 1 + NX] == 12)
                                                    CreateUNIT(fin, MakeXY_1(j + 1, i + 1, MY), P16[1], 0xBC);
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (j >= LX - 2)
                                            continue;
                                        int Type2[2] = { 0 };
                                        int Type = 0;
                                        if (j == LX - 3)
                                        {
                                            Type = 4;
                                            Type2[0] = 5;
                                            Type2[1] = 6;
                                        }
                                        else if (j == LX - 4)
                                        {
                                            Type = 3;
                                            Type2[0] = 4;
                                            Type2[1] = 5;
                                        }
                                        else if (j == LX - 5)
                                        {
                                            Type = 2;
                                            Type2[0] = 3;
                                            Type2[1] = 4;
                                        }
                                        else if (j == LX - 6)
                                        {
                                            Type = 1;
                                            Type2[0] = 2;
                                            Type2[1] = 3;
                                        }
                                        else if (j == LX - 7)
                                        {
                                            Type2[0] = 1;
                                            Type2[1] = 2;
                                        }
                                        else if (j == LX - 8)
                                        {
                                            Type2[1] = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index + 1 + NX])
                                        {
                                            if (IPtr[Index + 1 + NX] == IPtr[Index + 1])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check2(IPtr, NX, Index, Type) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 1 + NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_3(j + 1, i + 1, MY), IPtr[Index + 1 + NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_4(j + 1, i + 1, MY), IPtr[Index + 1 + NX], 0xD6);
                                                }
                                                else if (IPtr[Index + 1 + NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_2(j + 1, i + 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_5(j + 1, i + 1, MY), P16[1], 0xBC);
                                                }
                                                QCount++;
                                                CCount++;
                                            }
                                        }

                                        if (PCheck == 0 && Type <= 3 && IPtr[Index] != IPtr[Index + 2 + NX])
                                        {
                                            if (OCheck == 0 && IPtr[Index + 2 + NX] == IPtr[Index + 2])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check3(IPtr, NX, Index, Type2[0]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 2 + NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_3(j + 2, i + 1, MY), IPtr[Index + 2 + NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_4(j + 2, i + 1, MY), IPtr[Index + 2 + NX], 0xD6);
                                                }
                                                else if (IPtr[Index + 2 + NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_2(j + 2, i + 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_5(j + 2, i + 1, MY), P16[1], 0xBC);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index + 3 + NX])
                                        {
                                            if (OCheck == 0 && IPtr[Index + 3 + NX] == IPtr[Index + 3])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check4(IPtr, NX, Index, Type2[1]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 3 + NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_3(j + 3, i + 1, MY), IPtr[Index + 3 + NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_4(j + 3, i + 1, MY), IPtr[Index + 3 + NX], 0xD6);
                                                }
                                                else if (IPtr[Index + 3 + NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_2(j + 3, i + 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_5(j + 3, i + 1, MY), P16[1], 0xBC);
                                                }
                                                PCount[1]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (Dir == 2) // ↓→ 
                    {
                        for (int j = 0; j < LX - 1; j++)
                        {
                            for (int i = LY - 1; i > 0; i--)
                            {
                                int Index = i * NX + j;
                                if (DIM == 256 || DIM == 192)
                                {
                                    Flag = LCreateUNIT1B(fin, IPtr, DIM, LX, LY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index + NX + 2] = 3;
                                    if (FieldFlag[Index] == 3)
                                    {
                                        CreateUNITX(fin, MakeXY_1(j - 2, i + 1, MY), IPtr[Index + NX - 2], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] == 2)
                                        {
                                            CreateUNITX(fin, MakeXY_1(j - 2, i + 1, MY), IPtr[Index + NX - 2], P16[1]);
                                            FieldFlag[Index + NX] = 0;
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 2;
                                        else
                                            CreateUNITX(fin, MakeXY_1(j - 2, i, MY), IPtr[Index - 2], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] == 1)
                                        {
                                            CreateUNITX(fin, MakeXY_1(j - 1, i + 2, MY), IPtr[Index + 2 * NX - 1], P16[1]);
                                            FieldFlag[Index - 1] = 0;
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_1(j, i + 2, MY), IPtr[Index + 2 * NX], P16[1]);
                                    }
                                }
                                else if (DIM == 64)
                                {
                                    Flag = LCreateUNIT5B(fin, IPtr, DIM, LX, LY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index + NX + 4] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX1(fin, j - 4, i + 3, MY, IPtr[Index + 3*NX - 4], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (i <= LY-2 && FieldFlag[Index + NX] >= 4)
                                        {
                                            if (i >= 2 && IPtr[Index - NX] == 13 && FieldFlag[Index + NX] >= 4 && FieldFlag[Index + NX] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + NX];
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 4)
                                            {
                                                CreateUNITX1(fin, j - 4, i + 1, MY, IPtr[Index + NX - 4], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 5)
                                            {
                                                CreateUNITX1(fin, j - 4, i + 2, MY, IPtr[Index + 2*NX - 4], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 6)
                                            {
                                                CreateUNITX1(fin, j - 4, i + 3, MY, IPtr[Index + 3*NX - 4], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX1(fin, j-4, i, MY, IPtr[Index - 4], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 1)
                                        {
                                            if (j <= LX-3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] <= 2)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 1)
                                            {
                                                CreateUNITX1(fin, j - 1, i + 4, MY, IPtr[Index + 4*NX - 1], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 2)
                                            {
                                                CreateUNITX1(fin, j - 2, i + 4, MY, IPtr[Index + 4*NX - 2], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 3)
                                            {
                                                CreateUNITX1(fin, j - 3, i + 4, MY, IPtr[Index + 4*NX - 3], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX-3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX1(fin, j, i+4, MY, IPtr[Index + 4*NX], P16[1]);
                                    }
                                }
                                else
                                {
                                    Flag = LCreateUNIT9B(fin, IPtr, DIM, LX, LY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index + 4 + NX] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX(fin, MakeXY_3(j - 4, i + 2, MY), IPtr[Index - 4 + 2*NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 6)
                                        FieldFlag[Index + 4 + NX] = 8;
                                    if (FieldFlag[Index] == 8)
                                    {
                                        CreateUNITX(fin, MakeXY_2(j - 4, i + 1, MY), IPtr[Index - 4 + NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 4 && FieldFlag[Index - 1] <= 6)
                                        {
                                            if (j <= LX - 3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] >= 4 && FieldFlag[Index - 1] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 4)
                                            {
                                                CreateUNITX(fin, MakeXY_3(j - 1, i + 3, MY), IPtr[Index - 1 + 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 5)
                                            {
                                                CreateUNITX(fin, MakeXY_3(j - 2, i + 3, MY), IPtr[Index - 2 + 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 6)
                                            {
                                                CreateUNITX(fin, MakeXY_3(j - 3, i + 3, MY), IPtr[Index - 3 + 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX(fin, MakeXY_3(j, i + 3, MY), IPtr[Index + 3 * NX], P16[1]);
                                    }
                                    if (Flag == 5)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 9 && FieldFlag[Index - 1] <= 11)
                                        {
                                            if (j <= LX - 3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] >= 9 && FieldFlag[Index - 1] <= 10)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 9)
                                            {
                                                CreateUNITX(fin, MakeXY_2(j - 1, i + 2, MY), IPtr[Index - 1 + 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 10)
                                            {
                                                CreateUNITX(fin, MakeXY_2(j - 2, i + 2, MY), IPtr[Index - 2 + 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 11)
                                            {
                                                CreateUNITX(fin, MakeXY_2(j - 3, i + 2, MY), IPtr[Index - 3 + 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 9;
                                        else
                                            CreateUNITX(fin, MakeXY_2(j, i + 2, MY), IPtr[Index + 2 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] >= 1 && FieldFlag[Index + NX] <= 2)
                                        {
                                            if (i >= 2 && IPtr[Index - NX] == 13 && FieldFlag[Index + NX] <= 1)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + NX];
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 1)
                                            {
                                                CreateUNITX(fin, MakeXY_3(j - 4, i + 1, MY), IPtr[Index - 4 + NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 2)
                                            {
                                                CreateUNITX(fin, MakeXY_3(j - 4, i + 2, MY), IPtr[Index - 4 + 2 * NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_3(j - 4, i, MY), IPtr[Index - 4], P16[1]);
                                    }
                                    if (Flag == 4)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] >= 1 && FieldFlag[Index + NX] <= 2)
                                        {
                                            if (FieldFlag[Index + NX] == 1)
                                                CreateUNITX(fin, MakeXY_2(j - 4, i + 1, MY), IPtr[Index - 4 + NX], P16[1]);
                                            else
                                                CreateUNITX(fin, MakeXY_2(j - 4, i + 2, MY), IPtr[Index - 4 + 2 * NX], P16[1]);
                                            FieldFlag[Index + NX] = 0;
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_2(j - 4, i, MY), IPtr[Index - 4], P16[1]);
                                    }
                                }

                                if (MaskFlag[Index] == 1)
                                {
                                    if (j <= 0)
                                        continue;
                                    if (DIM == 256 || DIM == 192)
                                    {
                                        if (IPtr[Index] != IPtr[Index - 1 - NX])
                                        {
                                            if (i <= 1)
                                                continue;
                                            int Type = 0;
                                            if (i == 2)
                                                Type = 2;
                                            else if (i == 3)
                                                Type = 1;

                                            if (IPtr[Index - 1 - NX] == IPtr[Index - NX])
                                                OCount++;
                                            if (Check5(IPtr, NX, Index, Type) == 1)
                                            {
                                                if (IPtr[Index - 1 - NX] < 12)
                                                    CreateUNIT(fin, MakeXY_1(j - 1, i - 1, MY), IPtr[Index - 1 - NX], 0xD6);
                                                else if (IPtr[Index - 1 - NX] == 12)
                                                    CreateUNIT(fin, MakeXY_1(j - 1, i - 1, MY), P16[1], 0xBC);
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else if(DIM == 64)
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (i <= 1)
                                            continue;
                                        int Type2[2] = { 0 };
                                        int Type = 0;
                                        if (i == 2)
                                        {
                                            Type = 4;
                                            Type2[0] = 5;
                                            Type2[1] = 6;
                                        }
                                        else if (i == 3)
                                        {
                                            Type = 3;
                                            Type2[0] = 4;
                                            Type2[1] = 5;
                                        }
                                        else if (i == 4)
                                        {
                                            Type = 2;
                                            Type2[0] = 3;
                                            Type2[1] = 4;
                                        }
                                        else if (i == 5)
                                        {
                                            Type = 1;
                                            Type2[0] = 2;
                                            Type2[1] = 3;
                                        }
                                        else if (i == 6)
                                        {
                                            Type2[0] = 1;
                                            Type2[1] = 2;
                                        }
                                        else if (i == 7)
                                        {
                                            Type2[1] = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index - 1 - NX])
                                        {
                                            if (IPtr[Index - 1 - NX] == IPtr[Index - NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check6(IPtr, NX, Index, Type) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 1 - NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_3(j - 1, i - 1, MY), IPtr[Index - 1 - NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_4(j - 1, i - 1, MY), IPtr[Index - 1 - NX], 0xD6);
                                                }
                                                else if (IPtr[Index - 1 - NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_2(j - 1, i - 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_5(j - 1, i - 1, MY), P16[1], 0xBC);
                                                }
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 3 && IPtr[Index] != IPtr[Index - 2*NX - 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index - 2* NX - 1] == IPtr[Index - 2* NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check7(IPtr, NX, Index, Type2[0]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 2* NX - 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_3(j - 1, i - 2, MY), IPtr[Index - 2* NX - 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_4(j - 1, i - 2, MY), IPtr[Index - 2* NX - 1], 0xD6);
                                                }
                                                else if (IPtr[Index - 2* NX - 1] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_2(j - 1, i - 2, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_5(j - 1, i - 2, MY), P16[1], 0xBC);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index - 3* NX - 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index - 3* NX - 1] == IPtr[Index - 3* NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                                
                                            if (Check8(IPtr, NX, Index, Type2[1]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 3* NX - 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_3(j - 1, i - 3, MY), IPtr[Index - 3* NX - 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_4(j - 1, i - 3, MY), IPtr[Index - 3* NX - 1], 0xD6);
                                                }
                                                else if (IPtr[Index - 3* NX - 1] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_2(j - 1, i - 3, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_5(j - 1, i - 3, MY), P16[1], 0xBC);
                                                }
                                                PCount[1]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (i <= 1)
                                            continue;
                                        int Type2 = 0;
                                        int Type = 0;
                                        if (i == 2)
                                        {
                                            Type = 3;
                                            Type2 = 4;
                                        }
                                        else if (i == 3)
                                        {
                                            Type = 2;
                                            Type2 = 3;
                                        }
                                        else if (i == 4)
                                        {
                                            Type = 1;
                                            Type2 = 2;
                                        }
                                        else if (i == 5)
                                        {
                                            //Type = 1;
                                            Type2 = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index - 1 - NX])
                                        {
                                            if (IPtr[Index - 1 - NX] == IPtr[Index - NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (IPtr[Index - 1 - NX] < 12)
                                            {
                                                if (Check9(IPtr, NX, Index, Type) == 1)
                                                {
                                                    PCheck++;
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_3(j - 1, i - 1, MY), IPtr[Index - 1 - NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_4(j - 1, i - 1, MY), IPtr[Index - 1 - NX], 0xD6);
                                                    QCount++;
                                                    CCount++;
                                                }
                                            }
                                            else if (IPtr[Index - 1 - NX] == 12)
                                            {
                                                if (Check5(IPtr, NX, Index, Type) == 1)
                                                {
                                                    PCheck++;
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_2(j - 1, i - 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_5(j - 1, i - 1, MY), P16[1], 0xBC);
                                                    QCount++;
                                                    CCount++;
                                                }
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index - 2 * NX - 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index - 2 * NX - 1] == IPtr[Index - 2 * NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check0(IPtr, NX, Index, Type2) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 2 * NX - 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_3(j - 1, i - 2, MY), IPtr[Index - 2 * NX - 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_4(j - 1, i - 2, MY), IPtr[Index - 2 * NX - 1], 0xD6);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (Dir == 3) // ←↓ X = 1~127, Y = 1~127
                    {
                        int TX = 1;
                        if (DIM == 64 || DIM == 96 || DIM == 128)
                            TX = 3;
                        for (int i = LY - 1; i > 0; i--)
                        {
                            for (int j = LX - 1; j >= TX; j--)
                            {
                                int Index = i * NX + j;
                                if (DIM == 256 || DIM == 192)
                                {
                                    Flag = LCreateUNIT2A(fin, IPtr, DIM, LX, LY, TX, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index + 1 - 2 * NX] = 3;
                                    if (FieldFlag[Index] == 3)
                                    {
                                        CreateUNITX(fin, MakeXY_6(j + 1, i + 2, MY), IPtr[Index + 1 + 2 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j <= LX - 2 && FieldFlag[Index + 1] == 2)
                                        {
                                            CreateUNITX(fin, MakeXY_6(j + 1, i + 2, MY), IPtr[Index + 1 + 2 * NX], P16[1]);
                                            FieldFlag[Index + 1] = 0;
                                        }
                                        else if (j >= TX - 1 && IPtr[Index - 1] == 14)
                                            FieldFlag[Index] = 2;
                                        else
                                            CreateUNITX(fin, MakeXY_6(j, i + 2, MY), IPtr[Index + 2 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] == 1)
                                        {
                                            CreateUNITX(fin, MakeXY_6(j + 2, i + 1, MY), IPtr[Index + 2 + NX], P16[1]);
                                            FieldFlag[Index + NX] = 0;
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_6(j + 2, i, MY), IPtr[Index + 2], P16[1]);
                                    }
                                }
                                else if (DIM == 64)
                                {
                                    Flag = LCreateUNIT6A(fin, IPtr, DIM, LX, LY, TX, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index + 1 - 4 * NX] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX2(fin, j + 3, i + 4, MY, IPtr[Index + 3 + 4 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j <= LX-2 && FieldFlag[Index + 1] >= 4)
                                        {
                                            if (j >= TX - 1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] >= 4 && FieldFlag[Index + 1] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 4)
                                            {
                                                CreateUNITX2(fin, j + 1, i + 4, MY, IPtr[Index + 1 + 4 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 5)
                                            {
                                                CreateUNITX2(fin, j + 2, i + 4, MY, IPtr[Index + 2 + 4 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 6)
                                            {
                                                CreateUNITX2(fin, j + 3, i + 4, MY, IPtr[Index + 3 + 4 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                        }
                                        else if (j >= TX - 1 && IPtr[Index - 1] == 14)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX2(fin, j, i + 4, MY, IPtr[Index + 4 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] >= 1)
                                        {
                                            if (i >= 2 && IPtr[Index - NX] == 13 && FieldFlag[Index + NX] <= 2)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + NX];
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 1)
                                            {
                                                CreateUNITX2(fin, j + 4, i + 1, MY, IPtr[Index + 4 + NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 2)
                                            {
                                                CreateUNITX2(fin, j + 4, i + 2, MY, IPtr[Index + 4 + 2 * NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 3)
                                            {
                                                CreateUNITX2(fin, j + 4, i + 3, MY, IPtr[Index + 4 + 3 * NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX2(fin, j + 4, i, MY, IPtr[Index + 4], P16[1]);
                                    }
                                }
                                else
                                {
                                     Flag = LCreateUNIT0A(fin, IPtr, DIM, LX, LY, TX, i, j, MY, &CCount, P16[1]);

                                     if (Flag == 3)
                                         FieldFlag[Index + 1 - 3 * NX] = 7;
                                     if (FieldFlag[Index] == 7)
                                     {
                                         CreateUNITX(fin, MakeXY_8(j + 3, i + 3, MY), IPtr[Index + 3 + 3 * NX], P16[1]);
                                         FieldFlag[Index] = 0;
                                     }
                                     if (Flag == 6)
                                         FieldFlag[Index + 1 - 2 * NX] = 8;
                                     if (FieldFlag[Index] == 8)
                                     {
                                         CreateUNITX(fin, MakeXY_7(j + 3, i + 2, MY), IPtr[Index + 3 + 2 * NX], P16[1]);
                                         FieldFlag[Index] = 0;
                                     }
                                     if (Flag == 2)
                                     {
                                         if (j <= LX-2 && FieldFlag[Index + 1] >= 4 && FieldFlag[Index + 1] <= 6)
                                         {
                                             if (j >= TX-1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] >= 4 && FieldFlag[Index + 1] <= 5)
                                             {
                                                 FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 4)
                                             {
                                                 CreateUNITX(fin, MakeXY_8(j + 1, i + 3, MY), IPtr[Index + 1 + 3 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 5)
                                             {
                                                 CreateUNITX(fin, MakeXY_8(j + 2, i + 3, MY), IPtr[Index + 2 + 3 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 6)
                                             {
                                                 CreateUNITX(fin, MakeXY_8(j + 3, i + 3, MY), IPtr[Index + 3 + 3 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                         }
                                         else if (j >= TX-1 && IPtr[Index - 1] == 14)
                                             FieldFlag[Index] = 4;
                                         else
                                             CreateUNITX(fin, MakeXY_8(j, i + 3, MY), IPtr[Index + 3 * NX], P16[1]);
                                     }
                                     if (Flag == 5)
                                     {
                                         if (j <= LX-2 && FieldFlag[Index + 1] >= 9 && FieldFlag[Index + 1] <= 11)
                                         {
                                             if (j >= TX-1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] >= 9 && FieldFlag[Index + 1] <= 10)
                                             {
                                                 FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 9)
                                             {
                                                 CreateUNITX(fin, MakeXY_7(j + 1, i + 2, MY), IPtr[Index + 1 + 2 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 10)
                                             {
                                                 CreateUNITX(fin, MakeXY_7(j + 2, i + 2, MY), IPtr[Index + 2 + 2 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 11)
                                             {
                                                 CreateUNITX(fin, MakeXY_7(j + 3, i + 2, MY), IPtr[Index + 3 + 2 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                         }
                                         else if (j >= TX-1 && IPtr[Index - 1] == 14)
                                             FieldFlag[Index] = 9;
                                         else
                                             CreateUNITX(fin, MakeXY_7(j, i + 2, MY), IPtr[Index + 2 * NX], P16[1]);
                                     }
                                     if (Flag == 1)
                                     {
                                         if (i <= LY - 2 && FieldFlag[Index + NX] >= 1 && FieldFlag[Index + NX] <= 2)
                                         {
                                             if (i >= 2 && IPtr[Index - NX] == 13 && FieldFlag[Index + NX] <= 1)
                                             {
                                                 FieldFlag[Index] = 1 + FieldFlag[Index + NX];
                                                 FieldFlag[Index + NX] = 0;
                                             }
                                             else if (FieldFlag[Index + NX] == 1)
                                             {
                                                 CreateUNITX(fin, MakeXY_8(j + 4, i + 1, MY), IPtr[Index + 4 + NX], P16[1]);
                                                 FieldFlag[Index + NX] = 0;
                                             }
                                             else if (FieldFlag[Index + NX] == 2)
                                             {
                                                 CreateUNITX(fin, MakeXY_8(j + 4, i + 2, MY), IPtr[Index + 4 + 2 * NX], P16[1]);
                                                 FieldFlag[Index + NX] = 0;
                                             }
                                         }
                                         else if (i >= 2 && IPtr[Index - NX] == 13)
                                             FieldFlag[Index] = 1;
                                         else
                                             CreateUNITX(fin, MakeXY_8(j + 4, i, MY), IPtr[Index + 4], P16[1]);
                                     }
                                     if (Flag == 4)
                                     {
                                         if (i <= LY - 2 && FieldFlag[Index + NX] >= 1 && FieldFlag[Index + NX] <= 2)
                                         {
                                             if (FieldFlag[Index + NX] == 1)
                                                 CreateUNITX(fin, MakeXY_7(j + 4, i + 1, MY), IPtr[Index + 4 + NX], P16[1]);
                                             else
                                                 CreateUNITX(fin, MakeXY_7(j + 4, i + 2, MY), IPtr[Index + 4 + 2 * NX], P16[1]);
                                             FieldFlag[Index + NX] = 0;
                                         }
                                         else if (i >= 2 && IPtr[Index - NX] == 13)
                                             FieldFlag[Index] = 1;
                                         else
                                             CreateUNITX(fin, MakeXY_7(j + 4, i, MY), IPtr[Index + 4], P16[1]);
                                     }
                                }
                                
                                if (MaskFlag[Index] == 1)
                                {
                                    if (i >= LY - 1)
                                        continue;
                                    if (DIM == 256 || DIM == 192)
                                    {
                                        if (IPtr[Index] != IPtr[Index - 1 + NX])
                                        {
                                            if (j <= TX)
                                                continue;
                                            int Type = 0;
                                            if (j == TX+1)
                                                Type = 2;
                                            else if (j == TX+2)
                                                Type = 1;

                                            if (IPtr[Index - 1 + NX] == IPtr[Index - 1])
                                                OCount++;
                                            if (Check11(IPtr, NX, Index, Type) == 1)
                                            {
                                                if (IPtr[Index - 1 + NX] < 12)
                                                    CreateUNIT(fin, MakeXY_6(j - 1, i + 1, MY), IPtr[Index - 1 + NX], 0xD6);
                                                else if (IPtr[Index - 1 + NX] == 12)
                                                    CreateUNIT(fin, MakeXY_6(j - 1, i + 1, MY), P16[1], 0xBC);
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (j <= TX)
                                            continue;
                                        int Type2[2] = { 0 };
                                        int Type = 0;
                                        if (j == TX+1)
                                        {
                                            Type = 4;
                                            Type2[0] = 5;
                                            Type2[1] = 6;
                                        }
                                        else if (j == TX+2)
                                        {
                                            Type = 3;
                                            Type2[0] = 4;
                                            Type2[1] = 5;
                                        }
                                        else if (j == TX+3)
                                        {
                                            Type = 2;
                                            Type2[0] = 3;
                                            Type2[1] = 4;
                                        }
                                        else if (j == TX+4)
                                        {
                                            Type = 1;
                                            Type2[0] = 2;
                                            Type2[1] = 3;
                                        }
                                        else if (j == TX+5)
                                        {
                                            Type2[0] = 1;
                                            Type2[1] = 2;
                                        }
                                        else if (j == TX+6)
                                        {
                                            Type2[1] = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index - 1 + NX])
                                        {
                                            if (IPtr[Index - 1 + NX] == IPtr[Index - 1])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check12(IPtr, NX, Index, Type) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 1 + NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_8(j - 1, i + 1, MY), IPtr[Index - 1 + NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_9(j - 1, i + 1, MY), IPtr[Index - 1 + NX], 0xD6);
                                                }
                                                else if (IPtr[Index - 1 + NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_7(j - 1, i + 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_0(j - 1, i + 1, MY), P16[1], 0xBC);
                                                }
                                                QCount++;
                                                CCount++;
                                            }
                                        }

                                        if (PCheck == 0 && Type <= 3 && IPtr[Index] != IPtr[Index - 2 + NX])
                                        {
                                            if (OCheck == 0 && IPtr[Index - 2 + NX] == IPtr[Index - 2])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check13(IPtr, NX, Index, Type2[0]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 2 + NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_8(j - 2, i + 1, MY), IPtr[Index - 2 + NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_9(j - 2, i + 1, MY), IPtr[Index - 2 + NX], 0xD6);
                                                }
                                                else if (IPtr[Index - 2 + NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_7(j - 2, i + 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_0(j - 2, i + 1, MY), P16[1], 0xBC);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index - 3 + NX])
                                        {
                                            if (OCheck == 0 && IPtr[Index - 3 + NX] == IPtr[Index - 3])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check14(IPtr, NX, Index, Type2[1]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 3 + NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_8(j - 3, i + 1, MY), IPtr[Index - 3 + NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_9(j - 3, i + 1, MY), IPtr[Index - 3 + NX], 0xD6);
                                                }
                                                else if (IPtr[Index - 3 + NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_7(j - 3, i + 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_0(j - 3, i + 1, MY), P16[1], 0xBC);
                                                }
                                                PCount[1]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (Dir == 4) // ↓←
                    {
                        int TX = 1;
                        if (DIM == 64 || DIM == 96 || DIM == 128)
                            TX = 3;
                        for (int j = LX - 1; j >= TX; j--)
                        {
                            for (int i = LY - 1; i > 0; i--)
                            {
                                int Index = i * NX + j;
                                if (DIM == 256 || DIM == 192)
                                {
                                    Flag = LCreateUNIT2B(fin, IPtr, DIM, LX, LY, TX, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index + NX - 2] = 3;
                                    if (FieldFlag[Index] == 3)
                                    {
                                        CreateUNITX(fin, MakeXY_6(j + 2, i + 1, MY), IPtr[Index + NX + 2], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] == 2)
                                        {
                                            CreateUNITX(fin, MakeXY_6(j + 2, i + 1, MY), IPtr[Index + NX + 2], P16[1]);
                                            FieldFlag[Index + NX] = 0;
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 2;
                                        else
                                            CreateUNITX(fin, MakeXY_6(j + 2, i, MY), IPtr[Index + 2], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (j <= LX - 2 && FieldFlag[Index + 1] == 1)
                                        {
                                            CreateUNITX(fin, MakeXY_6(j + 1, i + 2, MY), IPtr[Index + 2 * NX + 1], P16[1]);
                                            FieldFlag[Index + 1] = 0;
                                        }
                                        else if (j >= TX - 1 && IPtr[Index - 1] == 14)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_6(j, i + 2, MY), IPtr[Index + 2 * NX], P16[1]);
                                    }
                                }
                                else if (DIM == 64)
                                {
                                    Flag = LCreateUNIT6B(fin, IPtr, DIM, LX, LY, TX, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index + NX - 4] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX2(fin, j + 4, i + 3, MY, IPtr[Index + 3 * NX + 4], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] >= 4)
                                        {
                                            if (i >= 2 && IPtr[Index - NX] == 13 && FieldFlag[Index + NX] >= 4 && FieldFlag[Index + NX] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + NX];
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 4)
                                            {
                                                CreateUNITX2(fin, j + 4, i + 1, MY, IPtr[Index + NX + 4], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 5)
                                            {
                                                CreateUNITX2(fin, j + 4, i + 2, MY, IPtr[Index + 2 * NX + 4], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 6)
                                            {
                                                CreateUNITX2(fin, j + 4, i + 3, MY, IPtr[Index + 3 * NX + 4], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX2(fin, j + 4, i, MY, IPtr[Index + 4], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (j <= LX-2 && FieldFlag[Index + 1] >= 1)
                                        {
                                            if (j >= TX - 1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] <= 2)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 1)
                                            {
                                                CreateUNITX2(fin, j + 1, i + 4, MY, IPtr[Index + 4 * NX + 1], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 2)
                                            {
                                                CreateUNITX2(fin, j + 2, i + 4, MY, IPtr[Index + 4 * NX + 2], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 3)
                                            {
                                                CreateUNITX2(fin, j + 3, i + 4, MY, IPtr[Index + 4 * NX + 3], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                        }
                                        else if (j <= TX - 1 && IPtr[Index - 1] == 14)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX2(fin, j, i + 4, MY, IPtr[Index + 4 * NX], P16[1]);
                                    }
                                }
                                else
                                {
                                    Flag = LCreateUNIT0B(fin, IPtr, DIM, LX, LY, TX, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index - 4 + NX] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX(fin, MakeXY_8(j + 4, i + 2, MY), IPtr[Index + 4 + 2 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 6)
                                        FieldFlag[Index - 4 + NX] = 8;
                                    if (FieldFlag[Index] == 8)
                                    {
                                        CreateUNITX(fin, MakeXY_7(j + 4, i + 1, MY), IPtr[Index + 4 + NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j <= LX-2 && FieldFlag[Index + 1] >= 4 && FieldFlag[Index + 1] <= 6)
                                        {
                                            if (j >= TX-1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] >= 4 && FieldFlag[Index + 1] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 4)
                                            {
                                                CreateUNITX(fin, MakeXY_8(j + 1, i + 3, MY), IPtr[Index + 1 + 3 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 5)
                                            {
                                                CreateUNITX(fin, MakeXY_8(j + 2, i + 3, MY), IPtr[Index + 2 + 3 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 6)
                                            {
                                                CreateUNITX(fin, MakeXY_8(j + 3, i + 3, MY), IPtr[Index + 3 + 3 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                        }
                                        else if (j >= TX-1 && IPtr[Index - 1] == 14)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX(fin, MakeXY_8(j, i + 3, MY), IPtr[Index + 3 * NX], P16[1]);
                                    }
                                    if (Flag == 5)
                                    {
                                        if (j <= LX-2 && FieldFlag[Index + 1] >= 9 && FieldFlag[Index + 1] <= 11)
                                        {
                                            if (j >= TX-1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] >= 9 && FieldFlag[Index + 1] <= 10)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 9)
                                            {
                                                CreateUNITX(fin, MakeXY_7(j + 1, i + 2, MY), IPtr[Index + 1 + 2 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 10)
                                            {
                                                CreateUNITX(fin, MakeXY_7(j + 2, i + 2, MY), IPtr[Index + 2 + 2 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 11)
                                            {
                                                CreateUNITX(fin, MakeXY_7(j + 3, i + 2, MY), IPtr[Index + 3 + 2 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                        }
                                        else if (j >= TX-1 && IPtr[Index - 1] == 14)
                                            FieldFlag[Index] = 9;
                                        else
                                            CreateUNITX(fin, MakeXY_7(j, i + 2, MY), IPtr[Index + 2 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] >= 1 && FieldFlag[Index + NX] <= 2)
                                        {
                                            if (i >= 2 && IPtr[Index - NX] == 13 && FieldFlag[Index + NX] <= 1)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + NX];
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 1)
                                            {
                                                CreateUNITX(fin, MakeXY_8(j + 4, i + 1, MY), IPtr[Index + 4 + NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                            else if (FieldFlag[Index + NX] == 2)
                                            {
                                                CreateUNITX(fin, MakeXY_8(j + 4, i + 2, MY), IPtr[Index + 4 + 2 * NX], P16[1]);
                                                FieldFlag[Index + NX] = 0;
                                            }
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_8(j + 4, i, MY), IPtr[Index + 4], P16[1]);
                                    }
                                    if (Flag == 4)
                                    {
                                        if (i <= LY - 2 && FieldFlag[Index + NX] >= 1 && FieldFlag[Index + NX] <= 2)
                                        {
                                            if (FieldFlag[Index + NX] == 1)
                                                CreateUNITX(fin, MakeXY_7(j + 4, i + 1, MY), IPtr[Index + 4 + NX], P16[1]);
                                            else
                                                CreateUNITX(fin, MakeXY_7(j + 4, i + 2, MY), IPtr[Index + 4 + 2 * NX], P16[1]);
                                            FieldFlag[Index + NX] = 0;
                                        }
                                        else if (i >= 2 && IPtr[Index - NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_7(j + 4, i, MY), IPtr[Index + 4], P16[1]);
                                    }
                                }
                                
                                if (MaskFlag[Index] == 1)
                                {
                                    if (j >= LX-1)
                                        continue;
                                    if (DIM == 256 || DIM == 192)
                                    {
                                        if (IPtr[Index] != IPtr[Index + 1 - NX])
                                        {
                                            if (i <= 1)
                                                continue;
                                            int Type = 0;
                                            if (i == 2)
                                                Type = 2;
                                            else if (i == 3)
                                                Type = 1;

                                            if (IPtr[Index + 1 - NX] == IPtr[Index - NX])
                                                OCount++;
                                            if (Check15(IPtr, NX, Index, Type) == 1)
                                            {
                                                if (IPtr[Index + 1 - NX] < 12)
                                                    CreateUNIT(fin, MakeXY_6(j + 1, i - 1, MY), IPtr[Index + 1 - NX], 0xD6);
                                                else if (IPtr[Index + 1 - NX] == 12)
                                                    CreateUNIT(fin, MakeXY_6(j + 1, i - 1, MY), P16[1], 0xBC);
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else if (DIM == 64)
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (i <= 1)
                                            continue;
                                        int Type2[2] = { 0 };
                                        int Type = 0;
                                        if (i == 2)
                                        {
                                            Type = 4;
                                            Type2[0] = 5;
                                            Type2[1] = 6;
                                        }
                                        else if (i == 3)
                                        {
                                            Type = 3;
                                            Type2[0] = 4;
                                            Type2[1] = 5;
                                        }
                                        else if (i == 4)
                                        {
                                            Type = 2;
                                            Type2[0] = 3;
                                            Type2[1] = 4;
                                        }
                                        else if (i == 5)
                                        {
                                            Type = 1;
                                            Type2[0] = 2;
                                            Type2[1] = 3;
                                        }
                                        else if (i == 6)
                                        {
                                            Type2[0] = 1;
                                            Type2[1] = 2;
                                        }
                                        else if (i == 7)
                                        {
                                            Type2[1] = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index + 1 - NX])
                                        {
                                            if (IPtr[Index + 1 - NX] == IPtr[Index - NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check16(IPtr, NX, Index, Type) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 1 - NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_8(j + 1, i - 1, MY), IPtr[Index + 1 - NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_9(j + 1, i - 1, MY), IPtr[Index + 1 - NX], 0xD6);
                                                }
                                                else if (IPtr[Index + 1 - NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_7(j + 1, i - 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_0(j + 1, i - 1, MY), P16[1], 0xBC);
                                                }
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 3 && IPtr[Index] != IPtr[Index - 2 * NX + 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index - 2 * NX + 1] == IPtr[Index - 2 * NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check17(IPtr, NX, Index, Type2[0]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 2 * NX + 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_8(j + 1, i - 2, MY), IPtr[Index - 2 * NX + 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_9(j + 1, i - 2, MY), IPtr[Index - 2 * NX + 1], 0xD6);
                                                }
                                                else if (IPtr[Index - 2 * NX + 1] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_7(j + 1, i - 2, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_0(j + 1, i - 2, MY), P16[1], 0xBC);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index - 3 * NX + 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index - 3 * NX + 1] == IPtr[Index - 3 * NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check18(IPtr, NX, Index, Type2[1]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 3 * NX + 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_8(j + 1, i - 3, MY), IPtr[Index - 3 * NX + 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_9(j + 1, i - 3, MY), IPtr[Index - 3 * NX + 1], 0xD6);
                                                }
                                                else if (IPtr[Index - 3 * NX + 1] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_7(j + 1, i - 3, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_0(j + 1, i - 3, MY), P16[1], 0xBC);
                                                }
                                                PCount[1]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (i <= 1)
                                            continue;
                                        int Type2 = 0;
                                        int Type = 0;
                                        if (i == 2)
                                        {
                                            Type = 3;
                                            Type2 = 4;
                                        }
                                        else if (i == 3)
                                        {
                                            Type = 2;
                                            Type2 = 3;
                                        }
                                        else if (i == 4)
                                        {
                                            Type = 1;
                                            Type2 = 2;
                                        }
                                        else if (i == 5)
                                        {
                                            //Type = 1;
                                            Type2 = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index + 1 - NX])
                                        {
                                            if (IPtr[Index + 1 - NX] == IPtr[Index - NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (IPtr[Index + 1 - NX] < 12)
                                            {
                                                if (Check19(IPtr, NX, Index, Type) == 1)
                                                {
                                                    PCheck++;
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_8(j + 1, i - 1, MY), IPtr[Index + 1 - NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_9(j + 1, i - 1, MY), IPtr[Index + 1 - NX], 0xD6);
                                                    QCount++;
                                                    CCount++;
                                                }
                                            }
                                            else if (IPtr[Index + 1 - NX] == 12)
                                            {
                                                if (Check15(IPtr, NX, Index, Type) == 1)
                                                {
                                                    PCheck++;
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_7(j + 1, i - 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_0(j + 1, i - 1, MY), P16[1], 0xBC);
                                                    QCount++;
                                                    CCount++;
                                                }
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index - 2 * NX + 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index - 2 * NX + 1] == IPtr[Index - 2 * NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check10(IPtr, NX, Index, Type2) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 2 * NX + 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_8(j + 1, i - 2, MY), IPtr[Index - 2 * NX + 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_9(j + 1, i - 2, MY), IPtr[Index - 2 * NX + 1], 0xD6);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (Dir == 5) // →↑ X = 0~126, Y = 0~126
                    {
                        int TY = 1;
                        if (DIM == 64)
                            TY = 3;
                        for (int i = 0; i < LY - TY; i++)
                        {
                            for (int j = 0; j < LX - 1; j++)
                            {
                                int Index = i * NX + j;
                                if (DIM == 256 || DIM == 192)
                                {
                                    Flag = LCreateUNIT3A(fin, IPtr, DIM, LX, LY, TY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index - 1 + 2 * NX] = 3;
                                    if (FieldFlag[Index] == 3)
                                    {
                                        CreateUNITX(fin, MakeXY_11(j - 1, i - 2, MY), IPtr[Index - 1 - 2 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] == 2)
                                        {
                                            CreateUNITX(fin, MakeXY_11(j - 1, i - 2, MY), IPtr[Index - 1 - 2 * NX], P16[1]);
                                            FieldFlag[Index - 1] = 0;
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 2;
                                        else
                                            CreateUNITX(fin, MakeXY_11(j, i - 2, MY), IPtr[Index - 2 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] == 1)
                                        {
                                            CreateUNITX(fin, MakeXY_11(j - 2, i - 1, MY), IPtr[Index - 2 - NX], P16[1]);
                                            FieldFlag[Index - NX] = 0;
                                        }
                                        else if (i <= LY - TY - 2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_11(j - 2, i, MY), IPtr[Index - 2], P16[1]);
                                    }
                                }
                                else if (DIM == 64)
                                {
                                    Flag = LCreateUNIT7A(fin, IPtr, DIM, LX, LY, TY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index - 1 + 4 * NX] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX3(fin, j - 3, i - 4, MY, IPtr[Index - 3 - 4 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 4)
                                        {
                                            if (j <= LX - 3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] >= 4 && FieldFlag[Index - 1] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 4)
                                            {
                                                CreateUNITX3(fin, j - 1, i - 4, MY, IPtr[Index - 1 - 4 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 5)
                                            {
                                                CreateUNITX3(fin, j - 2, i - 4, MY, IPtr[Index - 2 - 4 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 6)
                                            {
                                                CreateUNITX3(fin, j - 3, i - 4, MY, IPtr[Index - 3 - 4 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX3(fin, j, i - 4, MY, IPtr[Index - 4 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] >= 1)
                                        {
                                            if (i <= LY-TY-2 && IPtr[Index + NX] == 13 && FieldFlag[Index - NX] <= 2)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - NX];
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 1)
                                            {
                                                CreateUNITX3(fin, j - 4, i - 1, MY, IPtr[Index - 4 - NX], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 2)
                                            {
                                                CreateUNITX3(fin, j - 4, i - 2, MY, IPtr[Index - 4 - 2 * NX], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 3)
                                            {
                                                CreateUNITX3(fin, j - 4, i - 3, MY, IPtr[Index - 4 - 3 * NX], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                        }
                                        else if (i <= LY - TY - 2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX3(fin, j - 4, i, MY, IPtr[Index - 4], P16[1]);
                                    }
                                }
                                else
                                {
                                    Flag = LCreateUNITAA(fin, IPtr, DIM, LX, LY, TY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index - 1 + 3 * NX] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX(fin, MakeXY_13(j - 3, i - 3, MY), IPtr[Index - 3 - 3 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 6)
                                        FieldFlag[Index - 1 + 2 * NX] = 8;
                                    if (FieldFlag[Index] == 8)
                                    {
                                        CreateUNITX(fin, MakeXY_12(j - 3, i - 2, MY), IPtr[Index - 3 - 2 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 4 && FieldFlag[Index - 1] <= 6)
                                        {
                                            if (j <= LX - 3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] >= 4 && FieldFlag[Index - 1] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 4)
                                            {
                                                CreateUNITX(fin, MakeXY_13(j - 1, i - 3, MY), IPtr[Index - 1 - 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 5)
                                            {
                                                CreateUNITX(fin, MakeXY_13(j - 2, i - 3, MY), IPtr[Index - 2 - 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 6)
                                            {
                                                CreateUNITX(fin, MakeXY_13(j - 3, i - 3, MY), IPtr[Index - 3 - 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX(fin, MakeXY_13(j, i - 3, MY), IPtr[Index - 3 * NX], P16[1]);
                                    }
                                    if (Flag == 5)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 9 && FieldFlag[Index - 1] <= 11)
                                        {
                                            if (j <= LX - 3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] >= 9 && FieldFlag[Index - 1] <= 10)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 9)
                                            {
                                                CreateUNITX(fin, MakeXY_12(j - 1, i - 2, MY), IPtr[Index - 1 - 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 10)
                                            {
                                                CreateUNITX(fin, MakeXY_12(j - 2, i - 2, MY), IPtr[Index - 2 - 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 11)
                                            {
                                                CreateUNITX(fin, MakeXY_12(j - 3, i - 2, MY), IPtr[Index - 3 - 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 9;
                                        else
                                            CreateUNITX(fin, MakeXY_12(j, i - 2, MY), IPtr[Index - 2 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] >= 1 && FieldFlag[Index - NX] <= 2)
                                        {
                                            if (i <= LY-TY-2 && IPtr[Index + NX] == 13 && FieldFlag[Index - NX] <= 1)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - NX];
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 1)
                                            {
                                                CreateUNITX(fin, MakeXY_13(j - 4, i - 1, MY), IPtr[Index - 4 - NX], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 2)
                                            {
                                                CreateUNITX(fin, MakeXY_13(j - 4, i - 2, MY), IPtr[Index - 4 - 2 * NX], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                        }
                                        else if (i <= LY-TY-2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_13(j - 4, i, MY), IPtr[Index - 4], P16[1]);
                                    }
                                    if (Flag == 4)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] >= 1 && FieldFlag[Index - NX] <= 2)
                                        {
                                            if (FieldFlag[Index - NX] == 1)
                                                CreateUNITX(fin, MakeXY_12(j - 4, i - 1, MY), IPtr[Index - 4 - NX], P16[1]);
                                            else
                                                CreateUNITX(fin, MakeXY_12(j - 4, i - 2, MY), IPtr[Index - 4 - 2 * NX], P16[1]);
                                            FieldFlag[Index - NX] = 0;
                                        }
                                        else if (i <= LY-TY-2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_12(j - 4, i, MY), IPtr[Index - 4], P16[1]);
                                    }
                                }
                                
                                if (MaskFlag[Index] == 1)
                                {
                                    if (i <= 0)
                                        continue;
                                    if (DIM == 256 || DIM == 192)
                                    {
                                        if (IPtr[Index] != IPtr[Index + 1 - NX])
                                        {
                                            if (j >= LX - 2)
                                                continue;
                                            int Type = 0;
                                            if (j == LX - 3)
                                                Type = 2;
                                            else if (j == LX - 4)
                                                Type = 1;

                                            if (IPtr[Index + 1 - NX] == IPtr[Index + 1])
                                                OCount++;
                                            if (Check21(IPtr, NX, Index, Type) == 1)
                                            {
                                                if (IPtr[Index + 1 - NX] < 12)
                                                    CreateUNIT(fin, MakeXY_11(j + 1, i - 1, MY), IPtr[Index + 1 - NX], 0xD6);
                                                else if (IPtr[Index + 1 - NX] == 12)
                                                    CreateUNIT(fin, MakeXY_11(j + 1, i - 1, MY), P16[1], 0xBC);
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (j >= LX - 2)
                                            continue;
                                        int Type2[2] = { 0 };
                                        int Type = 0;
                                        if (j == LX - 3)
                                        {
                                            Type = 4;
                                            Type2[0] = 5;
                                            Type2[1] = 6;
                                        }
                                        else if (j == LX - 4)
                                        {
                                            Type = 3;
                                            Type2[0] = 4;
                                            Type2[1] = 5;
                                        }
                                        else if (j == LX - 5)
                                        {
                                            Type = 2;
                                            Type2[0] = 3;
                                            Type2[1] = 4;
                                        }
                                        else if (j == LX - 6)
                                        {
                                            Type = 1;
                                            Type2[0] = 2;
                                            Type2[1] = 3;
                                        }
                                        else if (j == LX - 7)
                                        {
                                            Type2[0] = 1;
                                            Type2[1] = 2;
                                        }
                                        else if (j == LX - 8)
                                        {
                                            Type2[1] = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index + 1 - NX])
                                        {
                                            if (IPtr[Index + 1 - NX] == IPtr[Index + 1])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check22(IPtr, NX, Index, Type) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 1 - NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_13(j + 1, i - 1, MY), IPtr[Index + 1 - NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_14(j + 1, i - 1, MY), IPtr[Index + 1 - NX], 0xD6);
                                                }
                                                else if (IPtr[Index + 1 - NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_12(j + 1, i - 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_15(j + 1, i - 1, MY), P16[1], 0xBC);
                                                }
                                                QCount++;
                                                CCount++;
                                            }
                                        }

                                        if (PCheck == 0 && Type <= 3 && IPtr[Index] != IPtr[Index + 2 - NX])
                                        {
                                            if (OCheck == 0 && IPtr[Index + 2 - NX] == IPtr[Index + 2])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check23(IPtr, NX, Index, Type2[0]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 2 - NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_13(j + 2, i - 1, MY), IPtr[Index + 2 - NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_14(j + 2, i - 1, MY), IPtr[Index + 2 - NX], 0xD6);
                                                }
                                                else if (IPtr[Index + 2 - NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_12(j + 2, i - 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_15(j + 2, i - 1, MY), P16[1], 0xBC);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index + 3 - NX])
                                        {
                                            if (OCheck == 0 && IPtr[Index + 3 - NX] == IPtr[Index + 3])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check24(IPtr, NX, Index, Type2[1]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 3 - NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_13(j + 3, i - 1, MY), IPtr[Index + 3 - NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_14(j + 3, i - 1, MY), IPtr[Index + 3 - NX], 0xD6);
                                                }
                                                else if (IPtr[Index + 3 - NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_12(j + 3, i - 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_15(j + 3, i - 1, MY), P16[1], 0xBC);
                                                }
                                                PCount[1]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (Dir == 6) // ↑→
                    {
                        int TY = 1;
                        if (DIM == 64)
                            TY = 3;
                        for (int j = 0; j < LX - 1; j++)
                        {
                            for (int i = 0; i < LY - TY; i++)
                            {
                                int Index = i * NX + j;
                                if (DIM == 256 || DIM == 192)
                                {
                                    Flag = LCreateUNIT3B(fin, IPtr, DIM, LX, LY, TY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index - NX + 2] = 3;
                                    if (FieldFlag[Index] == 3)
                                    {
                                        CreateUNITX(fin, MakeXY_11(j - 2, i - 1, MY), IPtr[Index - NX - 2], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] == 2)
                                        {
                                            CreateUNITX(fin, MakeXY_11(j - 2, i - 1, MY), IPtr[Index - NX - 2], P16[1]);
                                            FieldFlag[Index - NX] = 0;
                                        }
                                        else if (i <= LY - TY - 2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 2;
                                        else
                                            CreateUNITX(fin, MakeXY_11(j - 2, i, MY), IPtr[Index - 2], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] == 1)
                                        {
                                            CreateUNITX(fin, MakeXY_11(j - 1, i - 2, MY), IPtr[Index - 2 * NX - 1], P16[1]);
                                            FieldFlag[Index - 1] = 0;
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_11(j, i - 2, MY), IPtr[Index - 2 * NX], P16[1]);
                                    }
                                }
                                else if (DIM == 64)
                                {
                                    Flag = LCreateUNIT7B(fin, IPtr, DIM, LX, LY, TY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index - NX + 4] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX3(fin, j - 4, i - 3, MY, IPtr[Index - 3 * NX - 4], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] >= 4)
                                        {
                                            if (i <= LY-TY-2 && IPtr[Index + NX] == 13 && FieldFlag[Index - NX] >= 4 && FieldFlag[Index - NX] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - NX];
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 4)
                                            {
                                                CreateUNITX3(fin, j - 4, i - 1, MY, IPtr[Index - NX - 4], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 5)
                                            {
                                                CreateUNITX3(fin, j - 4, i - 2, MY, IPtr[Index - 2 * NX - 4], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 6)
                                            {
                                                CreateUNITX3(fin, j - 4, i - 3, MY, IPtr[Index - 3 * NX - 4], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                        }
                                        else if (i <= LY - TY - 2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX3(fin, j - 4, i, MY, IPtr[Index - 4], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 1)
                                        {
                                            if (j <= LX - 3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] <= 2)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 1)
                                            {
                                                CreateUNITX3(fin, j - 1, i - 4, MY, IPtr[Index - 4 * NX - 1], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 2)
                                            {
                                                CreateUNITX3(fin, j - 2, i - 4, MY, IPtr[Index - 4 * NX - 2], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 3)
                                            {
                                                CreateUNITX3(fin, j - 3, i - 4, MY, IPtr[Index - 4 * NX - 3], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX3(fin, j, i - 4, MY, IPtr[Index - 4 * NX], P16[1]);
                                    }
                                }
                                else
                                {
                                    Flag = LCreateUNITAB(fin, IPtr, DIM, LX, LY, TY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index + 4 - NX] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX(fin, MakeXY_13(j - 4, i - 2, MY), IPtr[Index - 4 - 2 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 6)
                                        FieldFlag[Index + 4 - NX] = 8;
                                    if (FieldFlag[Index] == 8)
                                    {
                                        CreateUNITX(fin, MakeXY_12(j - 4, i - 1, MY), IPtr[Index - 4 - NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 4 && FieldFlag[Index - 1] <= 6)
                                        {
                                            if (j <= LX - 3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] >= 4 && FieldFlag[Index - 1] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 4)
                                            {
                                                CreateUNITX(fin, MakeXY_13(j - 1, i - 3, MY), IPtr[Index - 1 - 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 5)
                                            {
                                                CreateUNITX(fin, MakeXY_13(j - 2, i - 3, MY), IPtr[Index - 2 - 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 6)
                                            {
                                                CreateUNITX(fin, MakeXY_13(j - 3, i - 3, MY), IPtr[Index - 3 - 3 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX(fin, MakeXY_13(j, i - 3, MY), IPtr[Index - 3 * NX], P16[1]);
                                    }
                                    if (Flag == 5)
                                    {
                                        if (j >= 1 && FieldFlag[Index - 1] >= 9 && FieldFlag[Index - 1] <= 11)
                                        {
                                            if (j <= LX - 3 && IPtr[Index + 1] == 14 && FieldFlag[Index - 1] >= 9 && FieldFlag[Index - 1] <= 10)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - 1];
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 9)
                                            {
                                                CreateUNITX(fin, MakeXY_12(j - 1, i - 2, MY), IPtr[Index - 1 - 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 10)
                                            {
                                                CreateUNITX(fin, MakeXY_12(j - 2, i - 2, MY), IPtr[Index - 2 - 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                            else if (FieldFlag[Index - 1] == 11)
                                            {
                                                CreateUNITX(fin, MakeXY_12(j - 3, i - 2, MY), IPtr[Index - 3 - 2 * NX], P16[1]);
                                                FieldFlag[Index - 1] = 0;
                                            }
                                        }
                                        else if (j <= LX - 3 && IPtr[Index + 1] == 14)
                                            FieldFlag[Index] = 9;
                                        else
                                            CreateUNITX(fin, MakeXY_12(j, i - 2, MY), IPtr[Index - 2 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] >= 1 && FieldFlag[Index - NX] <= 2)
                                        {
                                            if (i <= LY-TY-2 && IPtr[Index + NX] == 13 && FieldFlag[Index - NX] <= 1)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - NX];
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 1)
                                            {
                                                CreateUNITX(fin, MakeXY_13(j - 4, i - 1, MY), IPtr[Index - 4 - NX], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 2)
                                            {
                                                CreateUNITX(fin, MakeXY_13(j - 4, i - 2, MY), IPtr[Index - 4 - 2 * NX], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                        }
                                        else if (i <= LY-TY-2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_13(j - 4, i, MY), IPtr[Index - 4], P16[1]);
                                    }
                                    if (Flag == 4)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] >= 1 && FieldFlag[Index - NX] <= 2)
                                        {
                                            if (FieldFlag[Index - NX] == 1)
                                                CreateUNITX(fin, MakeXY_12(j - 4, i - 1, MY), IPtr[Index - 4 - NX], P16[1]);
                                            else
                                                CreateUNITX(fin, MakeXY_12(j - 4, i - 2, MY), IPtr[Index - 4 - 2 * NX], P16[1]);
                                            FieldFlag[Index - NX] = 0;
                                        }
                                        else if (i <= LY-TY-2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_12(j - 4, i, MY), IPtr[Index - 4], P16[1]);
                                    }
                                }
                                
                                if (MaskFlag[Index] == 1)
                                {
                                    if (j <= 0)
                                        continue;
                                    if (DIM == 256 || DIM == 192)
                                    {
                                        if (IPtr[Index] != IPtr[Index - 1 + NX])
                                        {
                                            if (i >= LY - TY - 1)
                                                continue;
                                            int Type = 0;
                                            if (i == LY - TY - 2)
                                                Type = 2;
                                            else if (i == LY - TY - 3)
                                                Type = 1;

                                            if (IPtr[Index - 1 + NX] == IPtr[Index + NX])
                                                OCount++;
                                            if (Check25(IPtr, NX, Index, Type) == 1)
                                            {
                                                if (IPtr[Index - 1 + NX] < 12)
                                                    CreateUNIT(fin, MakeXY_11(j - 1, i + 1, MY), IPtr[Index - 1 + NX], 0xD6);
                                                else if (IPtr[Index - 1 + NX] == 12)
                                                    CreateUNIT(fin, MakeXY_11(j - 1, i + 1, MY), P16[1], 0xBC);
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else if (DIM == 64)
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (i >= LY - TY - 1)
                                            continue;
                                        int Type2[2] = { 0 };
                                        int Type = 0;
                                        if (i == LY - TY - 2)
                                        {
                                            Type = 4;
                                            Type2[0] = 5;
                                            Type2[1] = 6;
                                        }
                                        else if (i == LY - TY - 3)
                                        {
                                            Type = 3;
                                            Type2[0] = 4;
                                            Type2[1] = 5;
                                        }
                                        else if (i == LY - TY - 4)
                                        {
                                            Type = 2;
                                            Type2[0] = 3;
                                            Type2[1] = 4;
                                        }
                                        else if (i == LY - TY - 5)
                                        {
                                            Type = 1;
                                            Type2[0] = 2;
                                            Type2[1] = 3;
                                        }
                                        else if (i == LY - TY - 6)
                                        {
                                            Type2[0] = 1;
                                            Type2[1] = 2;
                                        }
                                        else if (i == LY - TY - 7)
                                        {
                                            Type2[1] = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index - 1 + NX])
                                        {
                                            if (IPtr[Index - 1 + NX] == IPtr[Index + NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check26(IPtr, NX, Index, Type) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 1 + NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_13(j - 1, i + 1, MY), IPtr[Index - 1 + NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_14(j - 1, i + 1, MY), IPtr[Index - 1 + NX], 0xD6);
                                                }
                                                else if (IPtr[Index - 1 + NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_12(j - 1, i + 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_15(j - 1, i + 1, MY), P16[1], 0xBC);
                                                }
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 3 && IPtr[Index] != IPtr[Index + 2 * NX - 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index + 2 * NX - 1] == IPtr[Index + 2 * NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check27(IPtr, NX, Index, Type2[0]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 2 * NX - 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_13(j - 1, i + 2, MY), IPtr[Index + 2 * NX - 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_14(j - 1, i + 2, MY), IPtr[Index + 2 * NX - 1], 0xD6);
                                                }
                                                else if (IPtr[Index + 2 * NX - 1] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_12(j - 1, i + 2, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_15(j - 1, i + 2, MY), P16[1], 0xBC);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index + 3 * NX - 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index + 3 * NX - 1] == IPtr[Index + 3 * NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check28(IPtr, NX, Index, Type2[1]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 3 * NX - 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_13(j - 1, i + 3, MY), IPtr[Index + 3 * NX - 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_14(j - 1, i + 3, MY), IPtr[Index + 3 * NX - 1], 0xD6);
                                                }
                                                else if (IPtr[Index + 3 * NX - 1] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_12(j - 1, i + 3, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_15(j - 1, i + 3, MY), P16[1], 0xBC);
                                                }
                                                PCount[1]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (i >= LY - TY - 1)
                                            continue;
                                        int Type2 = 0;
                                        int Type = 0;
                                        if (i == LY - TY - 2)
                                        {
                                            Type = 3;
                                            Type2 = 4;
                                        }
                                        else if (i == LY - TY - 3)
                                        {
                                            Type = 2;
                                            Type2 = 3;
                                        }
                                        else if (i == LY - TY - 4)
                                        {
                                            Type = 1;
                                            Type2 = 2;
                                        }
                                        else if (i == LY - TY - 5)
                                        {
                                            //Type = 1;
                                            Type2 = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index - 1 + NX])
                                        {
                                            if (IPtr[Index - 1 + NX] == IPtr[Index + NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (IPtr[Index - 1 + NX] < 12)
                                            {
                                                if (Check29(IPtr, NX, Index, Type) == 1)
                                                {
                                                    PCheck++;
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_13(j - 1, i + 1, MY), IPtr[Index - 1 + NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_14(j - 1, i + 1, MY), IPtr[Index - 1 + NX], 0xD6);
                                                    QCount++;
                                                    CCount++;
                                                }
                                            }
                                            else if (IPtr[Index - 1 + NX] == 12)
                                            {
                                                if (Check25(IPtr, NX, Index, Type) == 1)
                                                {
                                                    PCheck++;
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_12(j - 1, i + 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_15(j - 1, i + 1, MY), P16[1], 0xBC);
                                                    QCount++;
                                                    CCount++;
                                                }
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index + 2 * NX - 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index + 2 * NX - 1] == IPtr[Index + 2 * NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check20(IPtr, NX, Index, Type2) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 2 * NX - 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_13(j - 1, i + 2, MY), IPtr[Index + 2 * NX - 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_14(j - 1, i + 2, MY), IPtr[Index + 2 * NX - 1], 0xD6);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (Dir == 7) // ←↑ X = 1~127, Y=0~126
                    {
                        int TX = 1;
                        if (DIM == 64 || DIM == 96 || DIM == 128)
                            TX = 3;
                        int TY = 1;
                        if (DIM == 64)
                            TY = 3;
                        for (int i = 0; i < LY - TY; i++)
                        {
                            for (int j = LX - 1; j >= TX; j--)
                            {
                                int Index = i * NX + j;
                                if (DIM == 256 || DIM == 192)
                                {
                                    Flag = LCreateUNIT4A(fin, IPtr, DIM, LX, LY, TX, TY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index + 1 + 2 * NX] = 3;
                                    if (FieldFlag[Index] == 3)
                                    {
                                        CreateUNITX(fin, MakeXY_16(j + 1, i - 2, MY), IPtr[Index + 1 - 2 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j <= LX - 2 && FieldFlag[Index + 1] == 2)
                                        {
                                            CreateUNITX(fin, MakeXY_16(j + 1, i - 2, MY), IPtr[Index + 1 - 2 * NX], P16[1]);
                                            FieldFlag[Index + 1] = 0;
                                        }
                                        else if (j >= TX - 1 && IPtr[Index - 1] == 14)
                                            FieldFlag[Index] = 2;
                                        else
                                            CreateUNITX(fin, MakeXY_16(j, i - 2, MY), IPtr[Index - 2 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] == 1)
                                        {
                                            CreateUNITX(fin, MakeXY_16(j + 2, i - 1, MY), IPtr[Index + 2 - NX], P16[1]);
                                            FieldFlag[Index - NX] = 0;
                                        }
                                        else if (i <= LY - TY - 2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_16(j + 2, i, MY), IPtr[Index + 2], P16[1]);
                                    }
                                }
                                else if (DIM == 64)
                                {
                                    Flag = LCreateUNIT8A(fin, IPtr, DIM, LX, LY, TX, TY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index + 1 + 4 * NX] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX4(fin, j + 3, i - 4, MY, IPtr[Index + 3 - 4 * NX], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (j <= LX-2 && FieldFlag[Index + 1] >= 4)
                                        {
                                            if (j >= TX-1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] >= 4 && FieldFlag[Index + 1] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 4)
                                            {
                                                CreateUNITX4(fin, j + 1, i - 4, MY, IPtr[Index + 1 - 4 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 5)
                                            {
                                                CreateUNITX4(fin, j + 2, i - 4, MY, IPtr[Index + 2 - 4 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 6)
                                            {
                                                CreateUNITX4(fin, j + 3, i - 4, MY, IPtr[Index + 3 - 4 * NX], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                        }
                                        else if (j >= TX - 1 && IPtr[Index - 1] == 14)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX4(fin, j, i - 4, MY, IPtr[Index - 4 * NX], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] >= 1)
                                        {
                                            if (i <= LY - TY - 2 && IPtr[Index + NX] == 13 && FieldFlag[Index - NX] <= 2)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - NX];
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 1)
                                            {
                                                CreateUNITX4(fin, j + 4, i - 1, MY, IPtr[Index + 4 - NX], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 2)
                                            {
                                                CreateUNITX4(fin, j + 4, i - 2, MY, IPtr[Index + 4 - 2 * NX], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 3)
                                            {
                                                CreateUNITX4(fin, j + 4, i - 3, MY, IPtr[Index + 4 - 3 * NX], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                        }
                                        else if (i <= LY - TY - 2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX4(fin, j + 4, i, MY, IPtr[Index + 4], P16[1]);
                                    }
                                }
                                else
                                {
                                     Flag = LCreateUNITBA(fin, IPtr, DIM, LX, LY, TX, TY, i, j, MY, &CCount, P16[1]);

                                     if (Flag == 3)
                                         FieldFlag[Index + 1 + 3 * NX] = 7;
                                     if (FieldFlag[Index] == 7)
                                     {
                                         CreateUNITX(fin, MakeXY_18(j + 3, i - 3, MY), IPtr[Index + 3 - 3 * NX], P16[1]);
                                         FieldFlag[Index] = 0;
                                     }
                                     if (Flag == 6)
                                         FieldFlag[Index + 1 + 2 * NX] = 8;
                                     if (FieldFlag[Index] == 8)
                                     {
                                         CreateUNITX(fin, MakeXY_17(j + 3, i - 2, MY), IPtr[Index + 3 - 2 * NX], P16[1]);
                                         FieldFlag[Index] = 0;
                                     }
                                     if (Flag == 2)
                                     {
                                         if (j <= LX-2 && FieldFlag[Index + 1] >= 4 && FieldFlag[Index + 1] <= 6)
                                         {
                                             if (j >= TX-1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] >= 4 && FieldFlag[Index + 1] <= 5)
                                             {
                                                 FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 4)
                                             {
                                                 CreateUNITX(fin, MakeXY_18(j + 1, i - 3, MY), IPtr[Index + 1 - 3 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 5)
                                             {
                                                 CreateUNITX(fin, MakeXY_18(j + 2, i - 3, MY), IPtr[Index + 2 - 3 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 6)
                                             {
                                                 CreateUNITX(fin, MakeXY_18(j + 3, i - 3, MY), IPtr[Index + 3 - 3 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                         }
                                         else if (j >= TX-1 && IPtr[Index - 1] == 14)
                                             FieldFlag[Index] = 4;
                                         else
                                             CreateUNITX(fin, MakeXY_18(j, i - 3, MY), IPtr[Index - 3 * NX], P16[1]);
                                     }
                                     if (Flag == 5)
                                     {
                                         if (j <= LX-2 && FieldFlag[Index + 1] >= 9 && FieldFlag[Index + 1] <= 11)
                                         {
                                             if (j >= TX-1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] >= 9 && FieldFlag[Index + 1] <= 10)
                                             {
                                                 FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 9)
                                             {
                                                 CreateUNITX(fin, MakeXY_17(j + 1, i - 2, MY), IPtr[Index + 1 - 2 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 10)
                                             {
                                                 CreateUNITX(fin, MakeXY_17(j + 2, i - 2, MY), IPtr[Index + 2 - 2 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                             else if (FieldFlag[Index + 1] == 11)
                                             {
                                                 CreateUNITX(fin, MakeXY_17(j + 3, i - 2, MY), IPtr[Index + 3 - 2 * NX], P16[1]);
                                                 FieldFlag[Index + 1] = 0;
                                             }
                                         }
                                         else if (j >= TX-1 && IPtr[Index - 1] == 14)
                                             FieldFlag[Index] = 9;
                                         else
                                             CreateUNITX(fin, MakeXY_17(j, i - 2, MY), IPtr[Index - 2 * NX], P16[1]);
                                     }
                                     if (Flag == 1)
                                     {
                                         if (i >= 1 && FieldFlag[Index - NX] >= 1 && FieldFlag[Index - NX] <= 2)
                                         {
                                             if (i <= LY - TY - 2 && IPtr[Index + NX] == 13 && FieldFlag[Index - NX] <= 1)
                                             {
                                                 FieldFlag[Index] = 1 + FieldFlag[Index - NX];
                                                 FieldFlag[Index - NX] = 0;
                                             }
                                             else if (FieldFlag[Index - NX] == 1)
                                             {
                                                 CreateUNITX(fin, MakeXY_18(j + 4, i - 1, MY), IPtr[Index + 4 - NX], P16[1]);
                                                 FieldFlag[Index - NX] = 0;
                                             }
                                             else if (FieldFlag[Index - NX] == 2)
                                             {
                                                 CreateUNITX(fin, MakeXY_18(j + 4, i - 2, MY), IPtr[Index + 4 - 2 * NX], P16[1]);
                                                 FieldFlag[Index - NX] = 0;
                                             }
                                         }
                                         else if (i <= LY - TY - 2 && IPtr[Index + NX] == 13)
                                             FieldFlag[Index] = 1;
                                         else
                                             CreateUNITX(fin, MakeXY_18(j + 4, i, MY), IPtr[Index + 4], P16[1]);
                                     }
                                     if (Flag == 4)
                                     {
                                         if (i >= 1 && FieldFlag[Index - NX] >= 1 && FieldFlag[Index - NX] <= 2)
                                         {
                                             if (FieldFlag[Index - NX] == 1)
                                                 CreateUNITX(fin, MakeXY_17(j + 4, i - 1, MY), IPtr[Index + 4 - NX], P16[1]);
                                             else
                                                 CreateUNITX(fin, MakeXY_17(j + 4, i - 2, MY), IPtr[Index + 4 - 2 * NX], P16[1]);
                                             FieldFlag[Index - NX] = 0;
                                         }
                                         else if (i <= LY-TY-2 && IPtr[Index + NX] == 13)
                                             FieldFlag[Index] = 1;
                                         else
                                             CreateUNITX(fin, MakeXY_17(j + 4, i, MY), IPtr[Index + 4], P16[1]);
                                     }
                                }
                                
                                if (MaskFlag[Index] == 1)
                                {
                                    if (i <= 0)
                                        continue;
                                    if (DIM == 256 || DIM == 192)
                                    {
                                        if (IPtr[Index] != IPtr[Index - 1 - NX])
                                        {
                                            if (j <= TX)
                                                continue;
                                            int Type = 0;
                                            if (j == TX+1)
                                                Type = 2;
                                            else if (j == TX+2)
                                                Type = 1;

                                            if (IPtr[Index - 1 - NX] == IPtr[Index - 1])
                                                OCount++;
                                            if (Check31(IPtr, NX, Index, Type) == 1)
                                            {
                                                if (IPtr[Index - 1 - NX] < 12)
                                                    CreateUNIT(fin, MakeXY_16(j - 1, i - 1, MY), IPtr[Index - 1 - NX], 0xD6);
                                                else if (IPtr[Index - 1 - NX] == 12)
                                                    CreateUNIT(fin, MakeXY_16(j - 1, i - 1, MY), P16[1], 0xBC);
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (j <= TX)
                                            continue;
                                        int Type2[2] = { 0 };
                                        int Type = 0;
                                        if (j == TX+1)
                                        {
                                            Type = 4;
                                            Type2[0] = 5;
                                            Type2[1] = 6;
                                        }
                                        else if (j == TX + 2)
                                        {
                                            Type = 3;
                                            Type2[0] = 4;
                                            Type2[1] = 5;
                                        }
                                        else if (j == TX + 3)
                                        {
                                            Type = 2;
                                            Type2[0] = 3;
                                            Type2[1] = 4;
                                        }
                                        else if (j == TX + 4)
                                        {
                                            Type = 1;
                                            Type2[0] = 2;
                                            Type2[1] = 3;
                                        }
                                        else if (j == TX + 5)
                                        {
                                            Type2[0] = 1;
                                            Type2[1] = 2;
                                        }
                                        else if (j == TX + 6)
                                        {
                                            Type2[1] = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index - 1 - NX])
                                        {
                                            if (IPtr[Index - 1 - NX] == IPtr[Index - 1])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check32(IPtr, NX, Index, Type) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 1 - NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_18(j - 1, i - 1, MY), IPtr[Index - 1 - NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_19(j - 1, i - 1, MY), IPtr[Index - 1 - NX], 0xD6);
                                                }
                                                else if (IPtr[Index - 1 - NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_17(j - 1, i - 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_10(j - 1, i - 1, MY), P16[1], 0xBC);
                                                }
                                                QCount++;
                                                CCount++;
                                            }
                                        }

                                        if (PCheck == 0 && Type <= 3 && IPtr[Index] != IPtr[Index - 2 - NX])
                                        {
                                            if (OCheck == 0 && IPtr[Index - 2 - NX] == IPtr[Index - 2])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check33(IPtr, NX, Index, Type2[0]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 2 - NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_18(j - 2, i - 1, MY), IPtr[Index - 2 - NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_19(j - 2, i - 1, MY), IPtr[Index - 2 - NX], 0xD6);
                                                }
                                                else if (IPtr[Index - 2 - NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_17(j - 2, i - 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_10(j - 2, i - 1, MY), P16[1], 0xBC);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index - 3 - NX])
                                        {
                                            if (OCheck == 0 && IPtr[Index - 3 - NX] == IPtr[Index - 3])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check34(IPtr, NX, Index, Type2[1]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index - 3 - NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_18(j - 3, i - 1, MY), IPtr[Index - 3 - NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_19(j - 3, i - 1, MY), IPtr[Index - 3 - NX], 0xD6);
                                                }
                                                else if (IPtr[Index - 3 - NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_17(j - 3, i - 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_10(j - 3, i - 1, MY), P16[1], 0xBC);
                                                }
                                                PCount[1]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (Dir == 8) // ↑←
                    {
                        int TX = 1;
                        if (DIM == 64 || DIM == 96 || DIM == 128)
                            TX = 3;
                        int TY = 1;
                        if (DIM == 64)
                            TY = 3;
                        for (int j = LX - 1; j >= TX; j--)
                        {
                            for (int i = 0; i < LY - TY; i++)
                            {
                                int Index = i * NX + j;
                                if (DIM == 256 || DIM == 192)
                                {
                                    Flag = LCreateUNIT4B(fin, IPtr, DIM, LX, LY, TX, TY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index - NX - 2] = 3;
                                    if (FieldFlag[Index] == 3)
                                    {
                                        CreateUNITX(fin, MakeXY_16(j + 2, i - 1, MY), IPtr[Index - NX + 2], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] == 2)
                                        {
                                            CreateUNITX(fin, MakeXY_16(j + 2, i - 1, MY), IPtr[Index - NX + 2], P16[1]);
                                            FieldFlag[Index - NX] = 0;
                                        }
                                        else if (i <= LY - TY - 2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 2;
                                        else
                                            CreateUNITX(fin, MakeXY_16(j + 2, i, MY), IPtr[Index + 2], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (j <= LX - 2 && FieldFlag[Index + 1] == 1)
                                        {
                                            CreateUNITX(fin, MakeXY_16(j + 1, i - 2, MY), IPtr[Index - 2 * NX + 1], P16[1]);
                                            FieldFlag[Index + 1] = 0;
                                        }
                                        else if (j >= TX - 1 && IPtr[Index - 1] == 14)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX(fin, MakeXY_16(j, i - 2, MY), IPtr[Index - 2 * NX], P16[1]);
                                    }
                                }
                                else if (DIM == 64)
                                {
                                    Flag = LCreateUNIT8B(fin, IPtr, DIM, LX, LY, TX, TY, i, j, MY, &CCount, P16[1]);

                                    if (Flag == 3)
                                        FieldFlag[Index - NX - 4] = 7;
                                    if (FieldFlag[Index] == 7)
                                    {
                                        CreateUNITX4(fin, j + 4, i - 3, MY, IPtr[Index - 3 * NX + 4], P16[1]);
                                        FieldFlag[Index] = 0;
                                    }
                                    if (Flag == 2)
                                    {
                                        if (i >= 1 && FieldFlag[Index - NX] >= 4)
                                        {
                                            if (i <= LY - TY - 2 && IPtr[Index + NX] == 13 && FieldFlag[Index - NX] >= 4 && FieldFlag[Index - NX] <= 5)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index - NX];
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 4)
                                            {
                                                CreateUNITX4(fin, j + 4, i - 1, MY, IPtr[Index - NX + 4], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 5)
                                            {
                                                CreateUNITX4(fin, j + 4, i - 2, MY, IPtr[Index - 2 * NX + 4], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                            else if (FieldFlag[Index - NX] == 6)
                                            {
                                                CreateUNITX4(fin, j + 4, i - 3, MY, IPtr[Index - 3 * NX + 4], P16[1]);
                                                FieldFlag[Index - NX] = 0;
                                            }
                                        }
                                        else if (i <= LY - TY - 2 && IPtr[Index + NX] == 13)
                                            FieldFlag[Index] = 4;
                                        else
                                            CreateUNITX4(fin, j + 4, i, MY, IPtr[Index + 4], P16[1]);
                                    }
                                    if (Flag == 1)
                                    {
                                        if (j <= LX-2 && FieldFlag[Index + 1] >= 1)
                                        {
                                            if (j >= TX-1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] <= 2)
                                            {
                                                FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 1)
                                            {
                                                CreateUNITX4(fin, j + 1, i - 4, MY, IPtr[Index - 4 * NX + 1], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 2)
                                            {
                                                CreateUNITX4(fin, j + 2, i - 4, MY, IPtr[Index - 4 * NX + 2], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                            else if (FieldFlag[Index + 1] == 3)
                                            {
                                                CreateUNITX4(fin, j + 3, i - 4, MY, IPtr[Index - 4 * NX + 3], P16[1]);
                                                FieldFlag[Index + 1] = 0;
                                            }
                                        }
                                        else if (j >= TX - 1 && IPtr[Index - 1] == 14)
                                            FieldFlag[Index] = 1;
                                        else
                                            CreateUNITX4(fin, j, i - 4, MY, IPtr[Index - 4 * NX], P16[1]);
                                    }
                                }
                                else
                                {
                                   Flag = LCreateUNITBB(fin, IPtr, DIM, LX, LY, TX, TY, i, j, MY, &CCount, P16[1]);

                                   if (Flag == 3)
                                       FieldFlag[Index - 4 - NX] = 7;
                                   if (FieldFlag[Index] == 7)
                                   {
                                       CreateUNITX(fin, MakeXY_18(j + 4, i - 2, MY), IPtr[Index + 4 - 2 * NX], P16[1]);
                                       FieldFlag[Index] = 0;
                                   }
                                   if (Flag == 6)
                                       FieldFlag[Index - 4 - NX] = 8;
                                   if (FieldFlag[Index] == 8)
                                   {
                                       CreateUNITX(fin, MakeXY_17(j + 4, i - 1, MY), IPtr[Index + 4 - NX], P16[1]);
                                       FieldFlag[Index] = 0;
                                   }
                                   if (Flag == 2)
                                   {
                                       if (j <= LX-2 && FieldFlag[Index + 1] >= 4 && FieldFlag[Index + 1] <= 6)
                                       {
                                           if (j >= TX-1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] >= 4 && FieldFlag[Index + 1] <= 5)
                                           {
                                               FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                               FieldFlag[Index + 1] = 0;
                                           }
                                           else if (FieldFlag[Index + 1] == 4)
                                           {
                                               CreateUNITX(fin, MakeXY_18(j + 1, i - 3, MY), IPtr[Index + 1 - 3 * NX], P16[1]);
                                               FieldFlag[Index + 1] = 0;
                                           }
                                           else if (FieldFlag[Index + 1] == 5)
                                           {
                                               CreateUNITX(fin, MakeXY_18(j + 2, i - 3, MY), IPtr[Index + 2 - 3 * NX], P16[1]);
                                               FieldFlag[Index + 1] = 0;
                                           }
                                           else if (FieldFlag[Index + 1] == 6)
                                           {
                                               CreateUNITX(fin, MakeXY_18(j + 3, i - 3, MY), IPtr[Index + 3 - 3 * NX], P16[1]);
                                               FieldFlag[Index + 1] = 0;
                                           }
                                       }
                                       else if (j >= TX-1  && IPtr[Index - 1] == 14)
                                           FieldFlag[Index] = 4;
                                       else
                                           CreateUNITX(fin, MakeXY_18(j, i - 3, MY), IPtr[Index - 3 * NX], P16[1]);
                                   }
                                   if (Flag == 5)
                                   {
                                       if (j <= LX-2 && FieldFlag[Index + 1] >= 9 && FieldFlag[Index + 1] <= 11)
                                       {
                                           if (j >= TX-1 && IPtr[Index - 1] == 14 && FieldFlag[Index + 1] >= 9 && FieldFlag[Index + 1] <= 10)
                                           {
                                               FieldFlag[Index] = 1 + FieldFlag[Index + 1];
                                               FieldFlag[Index + 1] = 0;
                                           }
                                           else if (FieldFlag[Index + 1] == 9)
                                           {
                                               CreateUNITX(fin, MakeXY_17(j + 1, i - 2, MY), IPtr[Index + 1 - 2 * NX], P16[1]);
                                               FieldFlag[Index + 1] = 0;
                                           }
                                           else if (FieldFlag[Index + 1] == 10)
                                           {
                                               CreateUNITX(fin, MakeXY_17(j + 2, i - 2, MY), IPtr[Index + 2 - 2 * NX], P16[1]);
                                               FieldFlag[Index + 1] = 0;
                                           }
                                           else if (FieldFlag[Index + 1] == 11)
                                           {
                                               CreateUNITX(fin, MakeXY_17(j + 3, i - 2, MY), IPtr[Index + 3 - 2 * NX], P16[1]);
                                               FieldFlag[Index + 1] = 0;
                                           }
                                       }
                                       else if (j >= TX-1 && IPtr[Index - 1] == 14)
                                           FieldFlag[Index] = 9;
                                       else
                                           CreateUNITX(fin, MakeXY_17(j, i - 2, MY), IPtr[Index - 2 * NX], P16[1]);
                                   }
                                   if (Flag == 1)
                                   {
                                       if (i >= 1 && FieldFlag[Index - NX] >= 1 && FieldFlag[Index - NX] <= 2)
                                       {
                                           if (i <= LY - TY - 2 && IPtr[Index + NX] == 13 && FieldFlag[Index - NX] <= 1)
                                           {
                                               FieldFlag[Index] = 1 + FieldFlag[Index - NX];
                                               FieldFlag[Index - NX] = 0;
                                           }
                                           else if (FieldFlag[Index - NX] == 1)
                                           {
                                               CreateUNITX(fin, MakeXY_18(j + 4, i - 1, MY), IPtr[Index + 4 - NX], P16[1]);
                                               FieldFlag[Index - NX] = 0;
                                           }
                                           else if (FieldFlag[Index - NX] == 2)
                                           {
                                               CreateUNITX(fin, MakeXY_18(j + 4, i - 2, MY), IPtr[Index + 4 - 2 * NX], P16[1]);
                                               FieldFlag[Index - NX] = 0;
                                           }
                                       }
                                       else if (i <= LY - TY - 2 && IPtr[Index + NX] == 13)
                                           FieldFlag[Index] = 1;
                                       else
                                           CreateUNITX(fin, MakeXY_18(j + 4, i, MY), IPtr[Index + 4], P16[1]);
                                   }
                                   if (Flag == 4)
                                   {
                                       if (i >= 1 && FieldFlag[Index - NX] >= 1 && FieldFlag[Index - NX] <= 2)
                                       {
                                           if (FieldFlag[Index - NX] == 1)
                                               CreateUNITX(fin, MakeXY_17(j + 4, i - 1, MY), IPtr[Index + 4 - NX], P16[1]);
                                           else
                                               CreateUNITX(fin, MakeXY_17(j + 4, i - 2, MY), IPtr[Index + 4 - 2 * NX], P16[1]);
                                           FieldFlag[Index - NX] = 0;
                                       }
                                       else if (i <= LY - TY - 2 && IPtr[Index + NX] == 13)
                                           FieldFlag[Index] = 1;
                                       else
                                           CreateUNITX(fin, MakeXY_17(j + 4, i, MY), IPtr[Index + 4], P16[1]);
                                   }
                                }
                                
                                if (MaskFlag[Index] == 1)
                                {
                                    if (j >= LX-1)
                                        continue;
                                    if (DIM == 256 || DIM == 192)
                                    {
                                        if (IPtr[Index] != IPtr[Index + 1 + NX])
                                        {
                                            if (i >= LY - TY - 1)
                                                continue;
                                            int Type = 0;
                                            if (i == LY - TY - 2)
                                                Type = 2;
                                            else if (i == LY - TY - 3)
                                                Type = 1;

                                            if (IPtr[Index + 1 + NX] == IPtr[Index + NX])
                                                OCount++;
                                            if (Check35(IPtr, NX, Index, Type) == 1)
                                            {
                                                if (IPtr[Index + 1 + NX] < 12)
                                                    CreateUNIT(fin, MakeXY_16(j + 1, i + 1, MY), IPtr[Index + 1 + NX], 0xD6);
                                                else if (IPtr[Index + 1 + NX] == 12)
                                                    CreateUNIT(fin, MakeXY_16(j + 1, i + 1, MY), P16[1], 0xBC);
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else if (DIM == 64)
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (i >= LY - TY - 1)
                                            continue;
                                        int Type2[2] = { 0 };
                                        int Type = 0;
                                        if (i == LY - TY - 2)
                                        {
                                            Type = 4;
                                            Type2[0] = 5;
                                            Type2[1] = 6;
                                        }
                                        else if (i == LY - TY - 3)
                                        {
                                            Type = 3;
                                            Type2[0] = 4;
                                            Type2[1] = 5;
                                        }
                                        else if (i == LY - TY - 4)
                                        {
                                            Type = 2;
                                            Type2[0] = 3;
                                            Type2[1] = 4;
                                        }
                                        else if (i == LY - TY - 5)
                                        {
                                            Type = 1;
                                            Type2[0] = 2;
                                            Type2[1] = 3;
                                        }
                                        else if (i == LY - TY - 6)
                                        {
                                            Type2[0] = 1;
                                            Type2[1] = 2;
                                        }
                                        else if (i == LY - TY - 7)
                                        {
                                            Type2[1] = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index + 1 + NX])
                                        {
                                            if (IPtr[Index + 1 + NX] == IPtr[Index + NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check36(IPtr, NX, Index, Type) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 1 + NX] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_18(j + 1, i + 1, MY), IPtr[Index + 1 + NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_19(j + 1, i + 1, MY), IPtr[Index + 1 + NX], 0xD6);
                                                }
                                                else if (IPtr[Index + 1 + NX] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_17(j + 1, i + 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_10(j + 1, i + 1, MY), P16[1], 0xBC);
                                                }
                                                QCount++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 3 && IPtr[Index] != IPtr[Index + 2 * NX + 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index + 2 * NX + 1] == IPtr[Index + 2 * NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check37(IPtr, NX, Index, Type2[0]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 2 * NX + 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_18(j + 1, i + 2, MY), IPtr[Index + 2 * NX + 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_19(j + 1, i + 2, MY), IPtr[Index + 2 * NX + 1], 0xD6);
                                                }
                                                else if (IPtr[Index + 2 * NX + 1] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_17(j + 1, i + 2, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_10(j + 1, i + 2, MY), P16[1], 0xBC);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index + 3 * NX + 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index + 3 * NX + 1] == IPtr[Index + 3 * NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (Check38(IPtr, NX, Index, Type2[1]) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 3 * NX + 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_18(j + 1, i + 3, MY), IPtr[Index + 3 * NX + 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_19(j + 1, i + 3, MY), IPtr[Index + 3 * NX + 1], 0xD6);
                                                }
                                                else if (IPtr[Index + 3 * NX + 1] == 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_17(j + 1, i + 3, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_10(j + 1, i + 3, MY), P16[1], 0xBC);
                                                }
                                                PCount[1]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                    else
                                    {
                                        int PCheck = 0, OCheck = 0;
                                        if (i >= LY - TY - 1)
                                            continue;
                                        int Type2 = 0;
                                        int Type = 0;
                                        if (i == LY - TY - 2)
                                        {
                                            Type = 3;
                                            Type2 = 4;
                                        }
                                        else if (i == LY - TY - 3)
                                        {
                                            Type = 2;
                                            Type2 = 3;
                                        }
                                        else if (i == LY - TY - 4)
                                        {
                                            Type = 1;
                                            Type2 = 2;
                                        }
                                        else if (i == LY - TY - 5)
                                        {
                                            //Type = 1;
                                            Type2 = 1;
                                        }

                                        if (IPtr[Index] != IPtr[Index + 1 + NX])
                                        {
                                            if (IPtr[Index + 1 + NX] == IPtr[Index + NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }

                                            if (IPtr[Index + 1 + NX] < 12)
                                            {
                                                if (Check39(IPtr, NX, Index, Type) == 1)
                                                {
                                                    PCheck++;
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_18(j + 1, i + 1, MY), IPtr[Index + 1 + NX], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_19(j + 1, i + 1, MY), IPtr[Index + 1 + NX], 0xD6);
                                                    QCount++;
                                                    CCount++;
                                                }
                                            }
                                            else if (IPtr[Index + 1 + NX] == 12)
                                            {
                                                if (Check35(IPtr, NX, Index, Type) == 1)
                                                {
                                                    PCheck++;
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_17(j + 1, i + 1, MY), P16[1], 0xBC);
                                                    else
                                                        CreateUNIT(fin, MakeXY_10(j + 1, i + 1, MY), P16[1], 0xBC);
                                                    QCount++;
                                                    CCount++;
                                                }
                                            }
                                        }
                                        if (PCheck == 0 && Type <= 2 && IPtr[Index] != IPtr[Index + 2 * NX + 1])
                                        {
                                            if (OCheck == 0 && IPtr[Index + 2 * NX + 1] == IPtr[Index + 2 * NX])
                                            {
                                                OCount++;
                                                OCheck++;
                                            }
                                            if (Check30(IPtr, NX, Index, Type2) == 1)
                                            {
                                                PCheck++;
                                                if (IPtr[Index + 2 * NX + 1] < 12)
                                                {
                                                    if (DIM == 128 || DIM == 96)
                                                        CreateUNIT(fin, MakeXY_18(j + 1, i + 2, MY), IPtr[Index + 2 * NX + 1], 0xD6);
                                                    else
                                                        CreateUNIT(fin, MakeXY_19(j + 1, i + 2, MY), IPtr[Index + 2 * NX + 1], 0xD6);
                                                }
                                                PCount[0]++;
                                                CCount++;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                   
                    if (Clear == 1 || Clear == 2)
                    {
                        if (OCount == 0)
                            printf("이미지에서 그림자가 감지되지 않았습니다\n\n");
                        else
                        {
                            if (DIM == 256 || DIM == 192)
                                printf("전체 %d개의 그림자 중 %d개 완전제거 완료 (%lf%%)\n\n", OCount, QCount, ((double)QCount / OCount) * 100);
                            else
                                printf("전체 %d개의 그림자 중 %d개 완전제거, %d개 부분제거 완료 (%lf%%)\n\n", OCount, QCount, PCount[0]+ PCount[1], (((double)QCount + (double)PCount[0]*0.666+ (double)PCount[1] * 0.333)/ OCount) * 100);
                        }
                    }

                    printf("Layer%d : %d개의 유닛이 추가되었습니다\n\n", LCount++, CCount);

                    printf("스타팅을 복구하려면 0을 입력하세요 (미 복구시 시작후 CenterView액션 필요) : ");
                    int Recover, RCount = 0;
                    scanf_s("%d", &Recover);
                    if (Recover == 0)
                    {
                        for (int l = 0; l < 8; l++)
                        {
                            if (Starting[l] != 0xFFFFFFFF)
                            {
                                CreateUNIT(fin, Starting[l], l, 0xD6);
                                RCount++;
                            }
                        }
                        printf("각 플레이어의 스타팅이 복구되었습니다 (Add %d Units)\n", RCount);
                    }
                }
                BEND:
                DeleteObject(HBMP);
            }
            fclose(fnew);
        }
    }

    fseek(fin, 0, 2);
    Size = ftell(fin);
    fseek(fin, 0x4, 0);
    DWORD UNITSize = Size - 0x8;
    fwrite(&UNITSize, 4, 1, fin);
    fseek(fin, 0x8, 0);

    printf("\nUNIT Section : All %d Units / 0x%X bytes\n\n", (Size - 0x8) / 0x24, Size - 0x8);
    return Size;
}


int main(int argc, char* argv[])
{	
    FILE* fout, * lout;
    FILE* wout[1024];
    MPQHANDLE hMPQ;
    MPQHANDLE hFile;
    MPQHANDLE hList;
    MPQHANDLE hWav[1024];
    char* WavName[1024];
    int Wavptr = 0;

    // Open MPQ

    printf("--------------------------------------\n     。`+˚CS_Minimap v5.0 。+.˚\n--------------------------------------\n\t\t\tMade By Ninfia\n");

    char* input = argv[1];
    //Test
    //char input2[] = "123.scm";
    //if (argc == 1)
    //  input = input2;
    //Test

    
    if (argc == 1) // Selected No file
    {
        printf("선택된 파일이 없습니다.\n");
        system("pause");
        return 0;
    }
    
    char iname[512];
    strcpy_s(iname, 512, input);
    int ilength = strlen(iname);

    iname[ilength - 4] = '_';
    iname[ilength - 3] = 'o';
    iname[ilength - 2] = 'u';
    iname[ilength - 1] = 't';
    iname[ilength - 0] = '.';
    iname[ilength + 1] = 's';
    iname[ilength + 2] = 'c';
    iname[ilength + 3] = 'x';
    iname[ilength + 4] = 0;

    if (!SFileOpenArchive(input, 0, 0, &hMPQ)) {
        // SFileOpenArchive([파일명], 0, 0, &[리턴받을 MPQ핸들]);
        printf("SFileOpenArchive failed. [%d]\n", GetLastError());
        return -1;
    }
    printf("%s 의 MPQ 로드 완료\n", input);
    // Open Files
    f_Sopen(hMPQ, "(listfile)", &hList);
    f_Scopy(hMPQ, &hList, "(listfile).txt", &lout);
    printf("(listfile)을 불러와 맵 내부의 파일 목록을 읽는중\n");
    fseek(lout, 0, 0);
    char strTemp[512] = { 0 };
    int strLength, listline, line;
    listline = getTotalLine(lout);
    line = 0;
    fseek(lout, 0, 0);
    int chksize = 0;
    while (line < listline)
    {
        line++;
        fgets(strTemp, 512, lout);
        strLength = strlen(strTemp);
        strTemp[strLength - 2] = 0;
        if (!strcmp(strTemp, "staredit\\scenario.chk"))
        {
            f_Sopen(hMPQ, "staredit\\scenario.chk", &hFile);
            f_Scopy(hMPQ, &hFile, "scenario.chk", &fout);
            fseek(fout, 0, 2);
            chksize = ftell(fout);
            fseek(fout, 0, 0);
            fclose(fout);
            SFileCloseFile(hFile);
        }
        else
        {
            char* tmpBuffer = (char*)malloc(512);
            tmpnam_s(tmpBuffer, 512);
            WavName[Wavptr] = tmpBuffer;
            f_Sopen(hMPQ, strTemp, &hWav[Wavptr]);
            f_Scopy(hMPQ, &hWav[Wavptr], WavName[Wavptr], &wout[Wavptr]);
            fclose(wout[Wavptr]);
            SFileCloseFile(hWav[Wavptr]);
            Wavptr++;
        }

    }
    fclose(lout);
    SFileCloseFile(hList);
    SFileCloseArchive(hMPQ); // MPQ 닫기

    printf("scenario.chk %d bytes, 사운드 %d개 추출됨\n\n", chksize, Wavptr);

    // Patch UNIT
    fopen_s(&fout, "scenario.chk", "rb");
    GetChkSection(fout, "UNIT");
    UNIToff1 = Ret1;
    UNIToff2 = Ret2;
    fseek(fout, 0, 2);
    UNIToff3 = ftell(fout);
    fseek(fout, 0, 0);
    //printf("%X %X %X\n", UNIToff1, UNIToff2, UNIToff3);
    DWORD UNITSizePrev = UNIToff2 - UNIToff1;

    FILE* fnew;
    fopen_s(&fnew, "scenario_new.chk", "wb");

    fseek(fout, 0, 0);
    f_Fcopy(&fout, &fnew, UNIToff1 - 0); // 0 ~ UNIT prev

    fseek(fout, UNIToff2, 0);
    f_Fcopy(&fout, &fnew, UNIToff3 - UNIToff2); // UNIT end ~ END

    FILE* UNIT;
    fopen_s(&UNIT, "UNIT.chk", "w+b");
    fseek(fout, UNIToff1, 0);
    f_Fcopy(&fout, &UNIT, UNIToff2 - UNIToff1);

    DWORD USize = PatchUNIT(fout,UNIT);

    fseek(UNIT, 0, 0);
    fseek(fnew, 0, 2);
    f_Fcopy(&UNIT, &fnew, USize); // Copy UNIT
    chksize = ftell(fnew);
    
    printf("출력할 썸네일 유닛파일 데이터의 이름을 입력하세요 (0 입력시 출력안함) : ");
    char Export[512] = { 0 };
    scanf_s("%s", Export, 512);
    if (strcmp(Export, "0"))
    {
        strcat_s(Export, 512, "_UNIT.chk");
        FILE* EXChk;
        fopen_s(&EXChk, Export, "w+b");
        fseek(UNIT, UNITSizePrev, 0);
        f_Fcopy(&UNIT, &EXChk, USize-UNITSizePrev);
        fclose(EXChk);
        printf("%s로 유닛파일 데이터가 출력됨 (0x%X bytes)\n\n",Export, USize - UNITSizePrev);
    }

    fclose(fout);
    fclose(fnew);
    fclose(UNIT);

    // Write MPQ
    char* out = iname;
    hMPQ = MpqOpenArchiveForUpdate(out, MOAU_CREATE_ALWAYS, 1024);
    if (hMPQ == INVALID_HANDLE_VALUE) { DeleteFileA(out);  return false; }

    // Write Files & Delete Temp
    f_Swrite(hMPQ, "(listfile).txt", "(listfile)");
    fopen_s(&lout, "(listfile).txt", "rb");

    Wavptr = 0;
    line = 0;
    while (line < listline)
    {
        line++;
        fgets(strTemp, 512, lout);
        strLength = strlen(strTemp);
        strTemp[strLength - 2] = 0;
        if (!strcmp(strTemp, "staredit\\scenario.chk"))
        {
            f_Swrite(hMPQ, "scenario_new.chk", "staredit\\scenario.chk");
            DeleteFileA("scenario.chk");
            DeleteFileA("scenario_new.chk");
            DeleteFileA("UNIT.chk");
        }
        else
        {
            f_SwriteWav(hMPQ, WavName[Wavptr], strTemp);
            DeleteFileA(WavName[Wavptr]);
            free(WavName[Wavptr]);
            Wavptr++;
        }
    }
    fclose(lout);
    DeleteFileA("(listfile).txt");
    MpqCloseUpdatedArchive(hMPQ, 0);

    printf("적용후 scenario.chk 의 크기 : %dbytes\n%s 로 저장됨\a\n", chksize, iname);

	system("pause");
	return 0;
}