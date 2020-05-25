/******************************************************************************
 * vtoydump_windows.c  ---- Dump ventoy os parameters in Windows
 *
 * Copyright (c) 2020, longpanda <admin@ventoy.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdio.h>
#include <stdint.h>
#include <vtoydump.h>

#include <Windows.h>
#include <virtdisk.h>
#include <winioctl.h>
#include <VersionHelpers.h>

static int verbose = 0;
static ventoy_guid vtoy_guid = VENTOY_GUID;

int vtoy_is_efi_system(void)
{
    UINT32 Data;
    DWORD Status;

    GetFirmwareEnvironmentVariableA("", "{00000000-0000-0000-0000-000000000000}", &Data, sizeof(Data));
    Status = GetLastError();

    debug("Get Dummy Firmware Environment Variable Status:%u\n", Status);

    if (ERROR_INVALID_FUNCTION == GetLastError())
    {
        return 0;
    }

    return 1;
}

//Grant Privilege
static int vtoy_grant_privilege(void)
{
    BOOL  NeedEnable = FALSE;
    DWORD i = 0;
    DWORD DataSize;
    DWORD Attributes;
    HANDLE hToken;
    CHAR PriName[512];
    TOKEN_PRIVILEGES *Privileges = NULL;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        debug("OpenProcessToken failed error:%u\n", GetLastError());
        return 1;
    }

    DataSize = 0;
    GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &DataSize);
    debug("GetTokenInformation Size needed:%u\n", DataSize);

    if (DataSize <= 0)
    {
        return 1;
    }

    Privileges = (TOKEN_PRIVILEGES *)malloc(DataSize);
    if (!Privileges)
    {
        return 1;
    }

    if (!GetTokenInformation(hToken, TokenPrivileges, Privileges, DataSize, &DataSize))
    {
        debug("GetTokenInformation failed error:%u size:%u\n", GetLastError(), DataSize);
        free(Privileges);
        return 1;
    }

    for (i = 0; i < Privileges->PrivilegeCount; i++)
    {
        DataSize = sizeof(PriName) / 2;
        memset(PriName, 0, sizeof(PriName));

        if (LookupPrivilegeNameA(NULL, &(Privileges->Privileges[i].Luid), PriName, &DataSize))
        {
            Attributes = Privileges->Privileges[i].Attributes;
            if (strncmp(PriName, "SeSystemEnvironmentPrivilege", strlen("SeSystemEnvironmentPrivilege")) == 0)
            {
                if ((Attributes & SE_PRIVILEGE_ENABLED) || (Attributes & SE_PRIVILEGE_ENABLED_BY_DEFAULT))
                {
                    debug("Privilege already enabled\n");
                }
                else
                {
                    debug("Privilege need to enable\n");
                    Privileges->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED;
                    NeedEnable = TRUE;
                }
                break;
            }
        }
    }

    if (NeedEnable)
    {
        if (AdjustTokenPrivileges(hToken, FALSE, Privileges, DataSize, NULL, NULL))
        {
            debug("AdjustTokenPrivileges success\n");
        }
        else
        {
            debug("AdjustTokenPrivileges failed errorcode:%u\n", GetLastError());
        }
    }

    free(Privileges);
    return 0;
}

int vtoy_os_param_from_efivar(ventoy_os_param *param)
{
    DWORD DataSize = 0;

    vtoy_grant_privilege();

    DataSize = GetFirmwareEnvironmentVariableA(VENTOY_VAR_NAME, VENTOY_GUID_STR, param, sizeof(ventoy_os_param));
    if (DataSize != sizeof(ventoy_os_param))
    {
        debug("GetFirmwareEnvironmentVariable failed, DataSize:%u ErrorCode:%u\n", DataSize, GetLastError());
        return 1;
    }

    debug("GetFirmwareEnvironmentVariable success\n");
    return 0;
}

int vtoy_os_param_from_ibft(ventoy_os_param *param)
{   
    UINT i = 0;
    UINT j = 0;
    UINT DataSize = 0;
    DWORD ErrorCode = 0;
    UINT8 DataBuf[1024];

    DataSize = GetSystemFirmwareTable('ACPI', 'TFBi', DataBuf, sizeof(DataBuf));
    ErrorCode = GetLastError();
    debug("get ACPI iBFT table size:%u error:%u\n", DataSize, ErrorCode);

    if (ErrorCode == ERROR_SUCCESS && DataSize > 0 && DataSize < sizeof(DataBuf))
    {
        for (j = 0; j < DataSize; j++)
        {
            if (0 == vtoy_check_os_param((ventoy_os_param *)(DataBuf + j)))
            {
                debug("find ventoy os pararm at iBFT offset %d\n", j);
                memcpy(param, DataBuf + j, sizeof(ventoy_os_param));
                return 0;
            }
        }
    }

    return 1;
}

static int vtoy_is_file_exist(const CHAR *FilePath)
{
    HANDLE hFile;

    hFile = CreateFileA(FilePath, FILE_READ_EA, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (INVALID_HANDLE_VALUE == hFile)
    {
        return 0;
    }

    CloseHandle(hFile);
    return 1;
}

#if 0
static int GetPhyDiskInfo(const char *LogicalDrive, UINT64 *pDiskSize, UINT8 UUID[16])
{
    BOOL Ret;
    DWORD dwSize;
    HANDLE Handle;
    VOLUME_DISK_EXTENTS DiskExtents;
    GET_LENGTH_INFORMATION LenInfo;
    CHAR *Pos = NULL;
    CHAR PhyPath[128];

    sprintf_s(PhyPath, sizeof(PhyPath), "\\\\.\\%s", LogicalDrive);
    Pos = strstr(PhyPath, ":");
    if (Pos)
    {
        *(Pos + 1) = 0;
    }

    Handle = CreateFileA(PhyPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (Handle == INVALID_HANDLE_VALUE)
    {
        debug("Could not open the disk<%s>, error:%u\n", PhyPath, GetLastError());
        return 1;
    }

    Ret = DeviceIoControl(Handle, 
                          IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, 
                          NULL, 
                          0, 
                          &DiskExtents,
                          (DWORD)(sizeof(DiskExtents)),
                          (LPDWORD)&dwSize, 
                          NULL);
    if (!Ret || DiskExtents.NumberOfDiskExtents == 0)
    {
        debug("DeviceIoControl IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed, error:%u\n", GetLastError());
        CloseHandle(Handle);
        return 1;
    }
    CloseHandle(Handle);

    sprintf_s(PhyPath, sizeof(PhyPath), "\\\\.\\PhysicalDrive%d", DiskExtents.Extents[0].DiskNumber);
    debug("%s is in PhysicalDrive%d \n", LogicalDrive, DiskExtents.Extents[0].DiskNumber);
    
    Handle = CreateFileA(PhyPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (Handle == INVALID_HANDLE_VALUE)
    {
        debug("Could not open the disk<%s>, error:%u\n", PhyPath, GetLastError());
        return 1;
    }

    Ret = DeviceIoControl(Handle, IOCTL_DISK_GET_LENGTH_INFO, 0, 0, &LenInfo, sizeof(LenInfo), &dwSize, 0);
    if (!Ret)
    {
        debug("DeviceIoControl IOCTL_DISK_GET_LENGTH_INFO failed, error:%u\n", GetLastError());
        CloseHandle(Handle);
        return 1;
    }

    ReadFile(Handle, 

    CloseHandle(Handle);
    *pDiskSize = LenInfo.Length.QuadPart;

    return 0;
}
#endif

static int GetPhyDiskUUID(const char *LogicalDrive, UINT8 UUID[16])
{
    BOOL Ret;
    DWORD dwSize;
    HANDLE Handle;
    VOLUME_DISK_EXTENTS DiskExtents;
    CHAR *Pos = NULL;
    CHAR PhyPath[128];
    UINT8 SectorBuf[512];

    sprintf_s(PhyPath, sizeof(PhyPath), "\\\\.\\%s", LogicalDrive);
    Pos = strstr(PhyPath, ":");
    if (Pos)
    {
        *(Pos + 1) = 0;
    }

    Handle = CreateFileA(PhyPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (Handle == INVALID_HANDLE_VALUE)
    {
        debug("Could not open the disk<%s>, error:%u\n", PhyPath, GetLastError());
        return 1;
    }

    Ret = DeviceIoControl(Handle,
        IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
        NULL,
        0,
        &DiskExtents,
        (DWORD)(sizeof(DiskExtents)),
        (LPDWORD)&dwSize,
        NULL);
    if (!Ret || DiskExtents.NumberOfDiskExtents == 0)
    {
        debug("DeviceIoControl IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS failed, error:%u\n", GetLastError());
        CloseHandle(Handle);
        return 1;
    }
    CloseHandle(Handle);

    sprintf_s(PhyPath, sizeof(PhyPath), "\\\\.\\PhysicalDrive%d", DiskExtents.Extents[0].DiskNumber);
    debug("%s is in PhysicalDrive%d \n", LogicalDrive, DiskExtents.Extents[0].DiskNumber);

    Handle = CreateFileA(PhyPath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    if (Handle == INVALID_HANDLE_VALUE)
    {
        debug("Could not open the disk<%s>, error:%u\n", PhyPath, GetLastError());
        return 1;
    }

    if (!ReadFile(Handle, SectorBuf, sizeof(SectorBuf), &dwSize, NULL))
    {
        debug("ReadFile failed, dwSize:%u  error:%u\n", dwSize, GetLastError());
        CloseHandle(Handle);
        return 1;
    }

    memcpy(UUID, SectorBuf + 0x180, 16);
    CloseHandle(Handle);
    return 0;
}

static void vtoy_dump_uuid(const char *prefix, UINT8 *uuid)
{
    int i;

    debug("%s\n", prefix);
    for (i = 0; i < 16; i++)
    {
        debug("%02X ", uuid[i]);
    }
}

int vtoy_find_disk(ventoy_os_param *param, char *diskname, int buflen)
{
    int rc = 1;
    DWORD DataSize = 0;
    CHAR *Pos = NULL; 
    CHAR *StringBuf = NULL;    
    UINT8 UUID[16];
    CHAR FilePath[512];

    vtoy_dump_uuid("Find disk for UUID: ", param->vtoy_disk_guid);
    debug("\n");

    DataSize = GetLogicalDriveStringsA(0, NULL);
    StringBuf = (CHAR *)malloc(DataSize + 1);
    if (StringBuf == NULL)
    {
        return 0;
    }

    GetLogicalDriveStringsA(DataSize, StringBuf);

    for (Pos = StringBuf; *Pos; Pos += strlen(Pos) + 1)
    {
        if (GetPhyDiskUUID(Pos, UUID) != 0)
        {
            continue;
        }

        debug("Check %s --> physical disk UUID: ", Pos);
        vtoy_dump_uuid("", UUID);

        if (memcmp(UUID, param->vtoy_disk_guid, sizeof(UUID)) != 0)
        {
            debug("Not Match\n");
            continue;
        }
        debug("OK\n");

        sprintf_s(diskname, buflen, "%s", Pos);
        
        sprintf_s(FilePath, sizeof(FilePath), "%s%s", diskname, param->vtoy_img_path + 1);
        if (!vtoy_is_file_exist(FilePath))
        {
            debug("File %s not exist ...\n", FilePath);
            continue;
        }
        else
        {
            debug("File %s exist ...\n", FilePath);
        }

        rc = 0;
        break;
    }

    free(StringBuf);
    return rc;
}

int vtoy_print_os_param(ventoy_os_param *param, char *diskname)
{
    printf("%s%s\n", diskname, param->vtoy_img_path + 1);
    return 0;
}

#ifdef VTOY_NT5
int vtoy_mount_iso(ventoy_os_param *param, const char *diskname)
{
    (void)param;
    (void)diskname;
    return 1;
}
#else
int vtoy_mount_iso(ventoy_os_param *param, const char *diskname)
{
    CHAR Drive = 'A';
    HANDLE Handle;
    DWORD Status;
    DWORD Drives0, Drives1;
    CHAR FilePath[512];
    WCHAR wFilePath[512] = { 0 };
    VIRTUAL_STORAGE_TYPE StorageType;
    OPEN_VIRTUAL_DISK_PARAMETERS OpenParameters;
    ATTACH_VIRTUAL_DISK_PARAMETERS AttachParameters;

    sprintf_s(FilePath, sizeof(FilePath), "%s%s", diskname, param->vtoy_img_path + 1);
    MultiByteToWideChar(CP_ACP, 0, FilePath, (int)strlen(FilePath), wFilePath, (int)(sizeof(wFilePath) / sizeof(WCHAR)));

    debug("mount iso file %s\n", FilePath);

    memset(&StorageType, 0, sizeof(StorageType));
    memset(&OpenParameters, 0, sizeof(OpenParameters));
    memset(&AttachParameters, 0, sizeof(AttachParameters));

    OpenParameters.Version = OPEN_VIRTUAL_DISK_VERSION_1;
    AttachParameters.Version = ATTACH_VIRTUAL_DISK_VERSION_1;

    Status = OpenVirtualDisk(&StorageType, wFilePath, VIRTUAL_DISK_ACCESS_READ, 0, &OpenParameters, &Handle);
    if (Status != ERROR_SUCCESS)
    {
        if (ERROR_VIRTDISK_PROVIDER_NOT_FOUND == Status)
        {
            printf("VirtualDisk for ISO file is not supported in current system\n");
        }
        else
        {
            printf("Failed to open virtual disk ErrorCode:%u\n", Status);
        }
        return 1;
    }

    debug("OpenVirtualDisk success\n");

    Drives0 = GetLogicalDrives();

    Status = AttachVirtualDisk(Handle, NULL, ATTACH_VIRTUAL_DISK_FLAG_READ_ONLY | ATTACH_VIRTUAL_DISK_FLAG_PERMANENT_LIFETIME, 0, &AttachParameters, NULL);
    if (Status != ERROR_SUCCESS)
    {
        printf("Failed to attach virtual disk ErrorCode:%u\n", Status);
        CloseHandle(Handle);
        return 1;        
    }
    
    debug("AttachVirtualDisk success\n");
    do
    {
        Sleep(100);
        Drives1 = GetLogicalDrives();
    } while (Drives1 == Drives0);

    Drives0 ^= Drives1;
    while ((Drives0 & 0x01) == 0)
    {
        Drive++;
        Drives0 >>= 1;
    }

    printf("%c: %s\n", Drive, FilePath);

    CloseHandle(Handle);
    return 0;
}
#endif

int vtoy_load_nt_driver(const char *DrvBinPath)
{
    int i;
    int rc = 0;
    BOOL Ret;
    DWORD Status;
    SC_HANDLE hServiceMgr;
    SC_HANDLE hService;
    char name[256] = { 0 };

    for (i = (int)strlen(DrvBinPath) - 1; i >= 0; i--)
    {
        if (DrvBinPath[i] == '\\' || DrvBinPath[i] == '/')
        {
            sprintf_s(name, sizeof(name), "%s", DrvBinPath + i + 1);
            break;
        }
    }

    debug("Load NT driver: %s %s\n", DrvBinPath, name);

    hServiceMgr = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hServiceMgr == NULL)
    {
        printf("OpenSCManager failed Error:%u\n", GetLastError());
        return 1;
    }

    debug("OpenSCManager OK\n");

    hService = CreateServiceA(hServiceMgr,
        name,
        name,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        DrvBinPath,
        NULL, NULL, NULL, NULL, NULL);
    if (hService == NULL)
    {
        Status = GetLastError();
        if (Status != ERROR_IO_PENDING && Status != ERROR_SERVICE_EXISTS)
        {
            printf("CreateService failed v %u\n", Status);
            CloseServiceHandle(hServiceMgr);
            return 1;
        }

        hService = OpenServiceA(hServiceMgr, name, SERVICE_ALL_ACCESS);
        if (hService == NULL)
        {
            printf("OpenService failed %u\n", Status);
            CloseServiceHandle(hServiceMgr);
            return 1;
        }
    }

    debug("CreateService imdisk OK\n");

    Ret = StartServiceA(hService, 0, NULL);
    if (Ret)
    {
        debug("StartService OK\n");
    }
    else
    {
        Status = GetLastError();
        if (Status == ERROR_SERVICE_ALREADY_RUNNING)
        {
            rc = 0;
        }
        else
        {
            debug("StartService error  %u\n", Status);
            rc = 1;
        }
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hServiceMgr);

    debug("Load NT driver %s\n", rc ? "failed" : "success");

    return rc;
}

void print_usage(void)
{
    printf("Usage: vtoydump [ -m ] [ -i filepath ] [ -v ]\n");
    printf("  none  Only print ventoy runtime data\n");
    printf("  -m    Mount the iso file \n"
           "        (Not supported before Windows 8 and Windows Server 2012)\n");
    printf("  -i    Load .sys driver\n");
    printf("  -v    Verbose, print additional debug info\n");
    printf("  -h    Print this help info\n");
    printf("\n");
}

int main(int argc, char **argv)
{
    int rc;
    int ch;
    char *pos;
    int mountiso = 0;
    char diskname[128] = { 0 };
    char filepath[256] = { 0 };
    ventoy_os_param param;

    for (ch = 1; ch < argc; ch++)
    {
        if (check_opt('m'))
        {
            mountiso = 1;           
        }   
        else if (check_opt('i'))
        {
            ch++;
            if (ch < argc && strstr(argv[ch], ":") && vtoy_is_file_exist(argv[ch]))
            {
                sprintf_s(filepath, sizeof(filepath), "%s", argv[ch]);
                return vtoy_load_nt_driver(filepath);
            }
            else
            {
                printf("Must input driver file full path\n");
                return 1;
            }
        }
        else if (check_opt('v'))
        {
            verbose = 1;
        }
        else if (check_opt('h'))
        {
            print_usage();
            return 0;
        }
        else
        {
            return 1;
        }
    }

    memset(&param, 0, sizeof(ventoy_os_param));

    if (vtoy_is_efi_system())
    {
        debug("current is efi system, get os pararm from efivar\n");
        rc = vtoy_os_param_from_efivar(&param);
    }
    else
    {
        debug("current is legacy bios system, get os pararm from ibft\n");
        rc = vtoy_os_param_from_ibft(&param);
    }

    if (rc)
    {
        fprintf(stderr, "ventoy runtime data not found\n");
        return rc;
    }

    // convert / to '\'
    for (pos = param.vtoy_img_path; *pos; pos++)
    {
        if (*pos == '/')
        {
            *pos = '\\';
        }
    }

    rc = vtoy_find_disk(&param, diskname, (int)(sizeof(diskname)-1));
    if (rc == 0)
    {
        if (mountiso)
        {
            if (IsWindows8OrGreater())
            {
                rc = vtoy_mount_iso(&param, diskname);
            }
            else
            {
                printf("VirtualDisk for ISO file is not supported before Windows 8\n");
                rc = 1;
            }
        }
        else
        {
            rc = vtoy_print_os_param(&param, diskname);
        }
    }

    return rc;
}

