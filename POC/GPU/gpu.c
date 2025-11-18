// sudo apt install libpci-dev
// gcc gpu.c -o /tmp/gpu.o -lpci && /tmp/gpu.o 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pci/pci.h>

// Base directory for Direct Rendering Manager devices
#define DRM_BASE_PATH "/sys/class/drm"
#define VENDOR_PATH "/vendor"
#define DEVICE_PATH "/device"

// PCI Class Code Constants
#define BASE_CLASS_DISPLAY_CONTROLLER 0x03
// PCI Vendor IDs
#define VENDOR_ID_AMD 0x1002
#define VENDOR_ID_INTEL 0x8086
#define VENDOR_ID_NVIDIA 0x10de

#define MAX_GPU 5

struct pci_access *pacc;


// --- Unified GPU Metrics Structure ---
typedef struct
{
    /*
    * Performance /sys/class/drm/card1/device/power_dpm_state
    * Memory clock /sys/class/drm/card1/device/pp_dpm_mclk
    */
    // General
    short utilization_gfx_percent; // Graphics/Compute Utilization (%)
    // /sys/class/drm/card1/device/gpu_busy_percent
    int power_watts;             // Power Consumption (W)
    // /sys/class/drm/card1/device/hwmon/hwmon2$ cat in0_input in1_input
    int temperature_celsius;     // GPU Die Temperature (°C)
    // /sys/class/drm/card1/device/hwmon/hwmon2/temp1_input

    // Video Engine
    int video_engine_percent; 
    // Video Encoder Utilization (%)
    // clock:  /sys/class/drm/card1/device/pp_dpm_vclk 
    // Video Decoder Utilization (%)
    // // clock:  /sys/class/drm/card1/device/pp_dpm_dclk
    // overall_percent
    // /sys/class/drm/card1/device/vcn_busy_percent

    // Frequency
    int core_clock_mhz; // Core/Shader Clock (MHz)
    // /sys/class/drm/card1/device/hwmon/hwmon2/freq1_input
    int mem_clock_mhz;  // Memory Clock (MHz)
    // /sys/class/drm/card1/device/pp_dpm_mclk

    // VRAM
    int vram_total_mb;     // Total VRAM (MB)
    // /sys/class/drm/card1/device/mem_info_vram_total
    int vram_used_mb;      // Used VRAM (MB)
    // /sys/class/drm/card1/device/mem_info_vram_used

    // Link & Driver
    char driver_used[64];      // e.g., "amdgpu" or "i915"
    char pcie_link_status[64]; // e.g., "x16 @ 8 GT/s"
    // /sys/class/drm/card1/device/current_link_speed, /sys/class/drm/card1/device/current_link_width

} GpuMetrics;

typedef struct
{
    char model_name[50];
    unsigned short vendor_id;
    unsigned short device_id;
    char sys_path[256];
    
    GpuMetrics metrics;
} GPUDevice;

void print_metrics(const GPUDevice *gpu)
{
    const GpuMetrics metrics = gpu->metrics;

    printf("\n======================================================\n");
    printf("GPU Device: %s\n", gpu->sys_path);
    printf("Driver: %s | Link: %s\n", metrics.driver_used, metrics.pcie_link_status);
    printf("======================================================\n");

    printf("--- Utilization ---\n");
    printf("  GFX/Compute: %hd%%\n", metrics.utilization_gfx_percent);
    printf("  Video Encoder/ Decoder:     %d%%\n", metrics.video_engine_percent);

    printf("--- Clocks & Power ---\n");
    printf("  Core Clock:  %d MHz\n", metrics.core_clock_mhz);
    printf("  Memory Clock: %d MHz\n", metrics.mem_clock_mhz);
    printf("  Power Draw:  %d W\n", metrics.power_watts);
    printf("  Temperature: %d °C\n", metrics.temperature_celsius);

    printf("--- VRAM ---\n");
    printf("  Total:       %d B\n", metrics.vram_total_mb);
    printf("  Used:        %d B\n", metrics.vram_used_mb);
}

/**
 * cat /usr/share/hwdata/pci.ids
 */
unsigned short get_vendor_id(const char *vendor_file_path)
{
    unsigned int vendor_id = 0;
    FILE *fp;

    fp = fopen(vendor_file_path, "r");
    if (fp == NULL)
        return 0;

    if (fscanf(fp, "%x", &vendor_id) == 1)
    {
        fclose(fp);
        return (unsigned short)vendor_id;
    }

    fclose(fp);
    return 0;
}

/*
read class file and return true if it has class_code of
*/
char is_GPU(const char *class_file_path)
{
    unsigned int class_code = 0;

    FILE *fp;

    fp = fopen(class_file_path, "r");
    if (fp == NULL)
        return 0;

    if (fscanf(fp, "%x", &class_code) == 1)
    {
        /*
        0-7 Programming Interface
        8 - 15 Subclass
        16 - 23 Base class
        */
        fclose(fp);
        unsigned short base_class = class_code >> 16;
        printf("Base class : %x\n", base_class);
        if (base_class == BASE_CLASS_DISPLAY_CONTROLLER)
        {
            unsigned short sub_class = (class_code & 0x00ff00) >> 8;
            printf("Sub class : %x\n", sub_class);
            unsigned short pci = (class_code & 0x0000ff);
            printf("pci class : %x\n", pci);
            return 1;
        }
    }
    return 0;
}

void get_dev_name(struct pci_access *pacc, char *buffer, int size, const unsigned short ven_id, const unsigned short dev_id)
{
    pci_lookup_name(pacc, buffer, size,
                    PCI_LOOKUP_DEVICE | PCI_LOOKUP_VENDOR,
                    ven_id, dev_id);
}

/*----------------S---READ GPU--------------------------*/

char get_gpu_busy_percentage(char *sys_path, void * buff){
    char tmp[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/gpu_busy_percent");
    FILE *fp;
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    char res = fscanf(fp, "%hd", (short *)buff);
    return res == 1 ?  1 :  0;
}

char get_video_engine_percentage(char *sys_path, void * buff) {
    char tmp[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/vcn_busy_percent");
    FILE *fp;
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    char res = fscanf(fp, "%hd", (short *)buff);
    return res == 1 ?  1 :  0;
}

char get_vram_total(char *sys_path, void * buff) {
    char tmp[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/mem_info_vram_total");
    FILE *fp;
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    char res = fscanf(fp, "%d", (int *)buff);
    return res == 1 ?  1 :  0;
}

char get_vram_used(char *sys_path, void * buff) {
    char tmp[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/mem_info_vram_used");
    FILE *fp;
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    char res = fscanf(fp, "%d", (int *)buff);
    return res == 1 ?  1 :  0;
}

/*----------------E---READ GPU--------------------------*/

void read_amd_metrics(GPUDevice * device){
    char path[256];
    snprintf(path, sizeof(path), "%s%s", device->sys_path, DEVICE_PATH);
    get_gpu_busy_percentage(path, &(device->metrics.utilization_gfx_percent));
    get_video_engine_percentage(path, &device->metrics.video_engine_percent);
    get_vram_total(path, &device->metrics.vram_total_mb);
    get_vram_used(path, &device->metrics.vram_used_mb);
}

int main()
{
    pacc = pci_alloc();
    if (!pacc)
    {
        fprintf(stderr, "Error: Could not allocate pci_access structure.\n");
        return 1;
    }

    pci_init(pacc);

    DIR *dir;
    struct dirent *entry;
    GPUDevice gpu_device[MAX_GPU];
    int i = 0;

    dir = opendir(DRM_BASE_PATH);
    if (dir == NULL)
    {
        fprintf(stderr, "Error opening directory %s: %s\n", DRM_BASE_PATH, strerror(errno));
        return 1;
    }

    while ((entry = readdir(dir)) != NULL || i >= MAX_GPU)
    {
        if (strncmp(entry->d_name, "card", 4) == 0)
        {
            char *num_start = entry->d_name + 4;
            if (*num_start != '\0' && strspn(num_start, "0123456789") == strlen(num_start))
            {
                int res = -1;
                char card_path[25];
                char tmp_path[100];

                res = snprintf(card_path, sizeof(card_path), "%s/%s", DRM_BASE_PATH, entry->d_name);

                if (res < 0)
                {
                    fprintf(stderr, "Error in card_path formattion %s: %s\n", DRM_BASE_PATH, strerror(errno));
                    return 1;
                }
                snprintf(tmp_path, sizeof(tmp_path), "%s/device/class", card_path);

                if (res < 0)
                {
                    fprintf(stderr, "Error in class_path formattion %s: %s\n", card_path, strerror(errno));
                    return 1;
                }

                if (is_GPU(tmp_path))
                {
                    strcpy(gpu_device[i].sys_path, card_path);
                    printf("Path : %s\n", gpu_device[i].sys_path);
                    snprintf(tmp_path, sizeof(tmp_path), "%s/device%s", card_path, VENDOR_PATH);
                    gpu_device[i].vendor_id = get_vendor_id(tmp_path);
                    printf("VENDOR id : %x\n", gpu_device[i].vendor_id);

                    snprintf(tmp_path, sizeof(tmp_path), "%s/device%s", card_path, DEVICE_PATH);
                    gpu_device[i].device_id = get_vendor_id(tmp_path);
                    printf("DEVICE id : %x\n", gpu_device[i].device_id);
                    get_dev_name(pacc, gpu_device[i].model_name, sizeof(gpu_device[i].model_name), gpu_device[i].vendor_id, gpu_device[i].device_id);
                    printf("DEVICE Name : %s\n", gpu_device[i].model_name);
                    if(gpu_device[i].vendor_id == VENDOR_ID_AMD){
                        read_amd_metrics(&gpu_device[i]);
                        print_metrics(&(gpu_device[i]));
                    }
                    i++;
                }
            }
        }
    }

    pci_cleanup(pacc);
    return 0;
}

/*
* Ref
-  https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/amd/include/kgd_pp_interface.h
-  https://dri.freedesktop.org/docs/drm/gpu/amdgpu/thermal.html#hwmon-interfaces
-  https://gist.github.com/leuc/e45f4dc64dc1db870e4bad1c436228bb
*/