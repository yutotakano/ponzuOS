#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/PrintLib.h>
#include <Protocol/LoadedImage.h>

// A structure for the memory map at boot
struct MemoryMap {
  UINTN buffer_size;
  VOID* buffer;
  UINTN map_size;
  UINTN map_key;
  UINTN descriptor_size;
  UINT32 descriptor_version;
};

EFI_STATUS GetMemoryMap(struct MemoryMap* map);
EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file);

// Get textual representation of the type of memory region
const CHAR16* GetMemoryTypeUnicode(EFI_MEMORY_TYPE type) {
  switch (type) {
    case EfiReservedMemoryType: return L"EfiReservedMemoryType";
    case EfiLoaderCode: return L"EfiLoaderCode";
    case EfiLoaderData: return L"EfiLoaderData";
    case EfiBootServicesCode: return L"EfiBootServicesCode";
    case EfiBootServicesData: return L"EfiBootServicesData";
    case EfiRuntimeServicesCode: return L"EfiRuntimeServicesCode";
    case EfiRuntimeServicesData: return L"EfiRuntimeServicesData";
    case EfiConventionalMemory: return L"EfiConventionalMemory";
    case EfiUnusableMemory: return L"EfiUnusableMemory";
    case EfiACPIReclaimMemory: return L"EfiACPIReclaimMemory";
    case EfiACPIMemoryNVS: return L"EfiACPIMemoryNVS";
    case EfiMemoryMappedIO: return L"EfiMemoryMappedIO";
    case EfiMemoryMappedIOPortSpace: return L"EfiMemoryMappedIOPortSpace";
    case EfiPalCode: return L"EfiPalCode";
    case EfiPersistentMemory: return L"EfiPersistentMemory";
    case EfiMaxMemoryType: return L"EfiMaxMemoryType";
    default: return L"InvalidMemoryType";
  }
}

// Open the root directory of the EFI image volume
EFI_STATUS OpenRootDir(EFI_HANDLE image_handle, EFI_FILE_PROTOCOL** root)
{
  EFI_LOADED_IMAGE_PROTOCOL* loaded_image;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;

  gBS->OpenProtocol(image_handle, &gEfiLoadedImageProtocolGuid, (VOID**)&loaded_image, image_handle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  gBS->OpenProtocol(loaded_image->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID**)&fs, image_handle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);

  fs->OpenVolume(fs, root);

  return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table) {
  Print(L"Hello, from PonzuLoader!\n");

  // Get memory map
  CHAR8 memorymap_buffer[4096 * 4];
  struct MemoryMap map = {sizeof(memorymap_buffer), memorymap_buffer, 0, 0, 0, 0};
  EFI_STATUS check = GetMemoryMap(&map);
  Print(L"Return value is %u\n", check);
  if (check != EFI_SUCCESS) {
    Print(L"Oh no, value is %u\n", check);
  }

  EFI_FILE_PROTOCOL* root_dir;
  OpenRootDir(image_handle, &root_dir);

  EFI_FILE_PROTOCOL *memmap_file;

  // Write to a file called memmap at root level -- to check this, mount disk.img
  // and ls inside it.
  root_dir->Open(root_dir, &memmap_file, L"\\memmap", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0);

  // Write memory map CSV to the file
  SaveMemoryMap(&map, memmap_file);
  memmap_file->Close(memmap_file);

  // Wait for a key press
  while(1);
  return EFI_SUCCESS;
}

EFI_STATUS GetMemoryMap(struct MemoryMap* map) {
  if (map->buffer == NULL) {
    return EFI_BUFFER_TOO_SMALL;
  }

  map->map_size = map->buffer_size;

  // gBS is the global boot EFI service (gRS is the runtime service)
  return gBS->GetMemoryMap(
    &map->map_size,
    (EFI_MEMORY_DESCRIPTOR*)map->buffer,
    &map->map_key,
    &map->descriptor_size,
    &map->descriptor_version
  );
}

EFI_STATUS SaveMemoryMap(struct MemoryMap* map, EFI_FILE_PROTOCOL* file)
{
  CHAR8 buf[256];
  UINTN len;

  // Write the header
  CHAR8* header = "Index, Type, Type(name), PhysicalStart, NumberOfPages, Attribute\n";
  len = AsciiStrLen(header);
  file->Write(file, &len, header);

  // Print overall stats about the memory map
  Print(L"map->buffer = %08lx, map->map_size = %08lx\n", map->buffer, map->map_size);

  EFI_PHYSICAL_ADDRESS iter;
  int i;

  // For each entry in the map, write it as a CSV row
  for (iter = (EFI_PHYSICAL_ADDRESS)map->buffer, i = 0;
       iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size;
       iter += map->descriptor_size, i++)
  {
    // Cast iter to a single memory descriptor
    EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;

    len = AsciiSPrint(buf, sizeof(buf), "%u, %x, %-ls, %08lx, %lx, %lx\n", i, desc->Type, GetMemoryTypeUnicode(desc->Type), desc->PhysicalStart, desc->NumberOfPages, desc->Attribute & 0xffffflu);

    file->Write(file, &len, buf);
  }

  return EFI_SUCCESS;
}
