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
#include <ctype.h>

// Base directory for Direct Rendering Manager devices
#define DRM_BASE_PATH "/sys/class/drm"
#define VENDOR_PATH "/vendor"
#define DEVICE_PATH "/device"

#define SCLK "OD_SCLK:\n"

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
    short utilization_gfx_percent; // Graphics/Compute Utilization (%) done
    // /sys/class/drm/card1/device/gpu_busy_percent
    float power_watts;             // Power Consumption (W)
    // /sys/class/drm/card1/device/hwmon/hwmon2$ cat in0_input in1_input // (mV) /1000 -> to get in volts
    float temperature_celsius;     // GPU Die Temperature (°C)
    // /sys/class/drm/card1/device/hwmon/hwmon2/temp1_input // millidegree Celsius /1000 -> to get in C

    // Video Engine
    short video_engine_percent; // done
    // Video Encoder Utilization (%)
    // clock:  /sys/class/drm/card1/device/pp_dpm_vclk 
    // Video Decoder Utilization (%)
    // // clock:  /sys/class/drm/card1/device/pp_dpm_dclk
    // overall_percent
    // /sys/class/drm/card1/device/vcn_busy_percent

    // Frequency . read * is active
    int core_mhz_range[2]; // o min, 1 max+1 // /sys/class/drm/card1/device/pp_od_clk_voltage -> OD_SCLK
    int memory_mhz_range[2]; // o min, 1 max+1 // /sys/class/drm/card1/device/pp_od_clk_voltage -> OD_MCLK
    int core_clock_mhz; // Core/Shader Clock (MHz) // pp_dpm_sclk
    // /sys/class/drm/card1/device/hwmon/hwmon2/freq1_input
    int mem_clock_mhz;  // Memory Clock (MHz)
    // /sys/class/drm/card1/device/pp_dpm_mclk

    // VRAM
    int vram_total_mb;     // Total VRAM (MB) // done
    // /sys/class/drm/card1/device/mem_info_vram_total
    int vram_used_mb;      // Used VRAM (MB) // done
    // /sys/class/drm/card1/device/mem_info_vram_used

    // Link & Driver
    char driver_used[64];      // e.g., "amdgpu" or "i915"
    short link_speed; // e.g., "x16 @ 8 GT/s"
    short link_width;
    // /sys/class/drm/card1/device/current_link_speed, /sys/class/drm/card1/device/current_link_width

} GpuMetrics;

typedef struct
{
    char model_name[50];
    unsigned short vendor_id;
    unsigned short device_id;
    char sys_path[256];
    short hwwom_num;
    GpuMetrics metrics;
} GPUDevice;

void print_metrics(const GPUDevice *gpu)
{
    const GpuMetrics metrics = gpu->metrics;

    printf("\n======================================================\n");
    printf("GPU Device: %s\n", gpu->sys_path);
    printf("Driver: %s | Link: %hd GT/s x %hd\n", metrics.driver_used, metrics.link_speed, metrics.link_width);
    printf("Device Name : %s\n", gpu->model_name);
    printf("======================================================\n");
    printf("--- Range ---\n");
    printf("  GPU Min: %d MHz and Max: %d MHz\n", metrics.core_mhz_range[0], metrics.core_mhz_range[1]);
    printf("  Mem Min: %d MHz and Max: %d MHz\n", metrics.memory_mhz_range[0], metrics.memory_mhz_range[1]);
    printf("--- Utilization ---\n");
    printf("  GFX/Compute: %hd%%\n", metrics.utilization_gfx_percent);
    printf("  Video Encoder/ Decoder:     %hd%%\n", metrics.video_engine_percent);

    printf("--- Clocks & Power ---\n");
    printf("  Core Clock:  %d MHz\n", metrics.core_clock_mhz);
    printf("  Memory Clock: %d MHz\n", metrics.mem_clock_mhz);
    printf("  Power Draw:  %f W\n", metrics.power_watts);
    printf("  Temperature: %f °C\n", metrics.temperature_celsius);

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
        // printf("Base class : %x\n", base_class);
        if (base_class == BASE_CLASS_DISPLAY_CONTROLLER)
        {
            unsigned short sub_class = (class_code & 0x00ff00) >> 8;
            // printf("Sub class : %x\n", sub_class);
            unsigned short pci = (class_code & 0x0000ff);
            // printf("pci class : %x\n", pci);
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
char get_link_width( char *sys_path, short * buff){
    char tmp[256];
    short link_width;
    snprintf(tmp, 256, "%s%s", sys_path, "/current_link_width");
    FILE *fp;
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    int res = fscanf(fp, "%hd", buff);
    fclose(fp);
    return res == 1 ?  1 :  0;
}

char get_link_seed( char *sys_path, short * buff){
    char tmp[256];
    short link_speed;
    snprintf(tmp, 256, "%s%s", sys_path, "/current_link_speed");
    FILE *fp;
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    int res = fscanf(fp, "%hd", buff);
    fclose(fp);
    return res == 1 ?  1 :  0;
}
char get_driver_name(char *sys_path, char * dest){
    char tmp[256];
    char sym[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/driver");

    int res = readlink(tmp, sym, sizeof(sym)-1);
    if(res != -1) {
        sym[res] = '\0';
        strcpy(dest, strrchr(sym, '/')+1);
    }
    return 1;
}
char get_sclk_range(char *sys_path, int (*buff)[2]){
    char tmp[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/pp_od_clk_voltage");

    FILE *fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    (*buff)[0]=-1;
    (*buff)[1]=-1;
    char is_sclk=0;
    int t = -1;
    // read line by line
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;

    while ((read = getline(&line, &len, fp)) != -1) {
        
        if(!isdigit(line[0]) && is_sclk == 1) {
            is_sclk = 0;
        }
        if(strcmp(line, SCLK) == 0){
            is_sclk = 1;
            continue;
        }
        if(is_sclk == 1) {
            sscanf(line, "%*[^:]%*[^0-9]%d", &t);
            if ((*buff)[0] == -1 || t < (*buff)[0]) (*buff)[0] = t;
            if (t > (*buff)[1]) (*buff)[1] = t;
        }
    }

    fclose(fp);
    return 1;
}

char get_mclk_range(char *sys_path, int (*buff)[2], int * current){
char tmp[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/pp_dpm_mclk");

    FILE *fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int t = -1;
    (*buff)[0]=-1;
    (*buff)[1]=-1;

    // read line by line
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    while ((read = getline(&line, &len, fp)) != -1) {
        sscanf(line, "%*[^:]%*[^0-9]%d", &t);
        if(line[read-2]=='*'){
            *current = t;
        }
        if ((*buff)[0] == -1 || t < (*buff)[0]) (*buff)[0] = t;
            if (t > (*buff)[1]) (*buff)[1] = t;
    }
    fclose(fp);
    return 1;
}

char get_gpu_clock(char *sys_path, int * buff){
    char tmp[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/freq1_input");
    FILE *fp;
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    long long hz = 0; 
    int res = fscanf(fp, "%lld", &hz);
    *buff = hz/1000000;
    fclose(fp);
    return res == 1 ?  1 :  0;
}

char get_gpu_watts(char *sys_path, float * buff){
    char tmp[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/power1_average");
    FILE *fp;
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    long long microwatts = 0; 
    int res = fscanf(fp, "%lld", &microwatts);
    *buff = microwatts/1000000.0f;
    fclose(fp);
    return res == 1 ?  1 :  0;
}

char get_gpu_temp(char *sys_path, float * buff){
    char tmp[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/temp1_input");
    FILE *fp;
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    int milliocelcius = 0; 
    int res = fscanf(fp, "%d", &milliocelcius);
    *buff = milliocelcius/1000.0f;
    fclose(fp);
    return res == 1 ?  1 :  0;
}

char get_gpu_busy_percentage(char *sys_path, short * buff){
    char tmp[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/gpu_busy_percent");
    FILE *fp;
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    char res = fscanf(fp, "%hd", buff);
    fclose(fp);
    return res == 1 ?  1 :  0;
}

char get_video_engine_percentage(char *sys_path, short * buff) {
    char tmp[256];
    snprintf(tmp, 256, "%s%s", sys_path, "/vcn_busy_percent");
    FILE *fp;
    fp = fopen(tmp, "r");
    if (fp == NULL)
        return 0;
    char res = fscanf(fp, "%hd", buff);
    fclose(fp);
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
    fclose(fp);
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
    fclose(fp);
    return res == 1 ?  1 :  0;
}

char get_hw_num(char *sys_path, short *buff){
    int i = 0;
    DIR *dir;
    struct dirent *entry; 
    dir = opendir(sys_path);
    if (dir == NULL){
        fprintf(stderr, "Error opening directory %s: %s\n", sys_path, strerror(errno));
        return 0;
    }

    while((entry = readdir(dir)) != NULL ){
        if (strncmp(entry->d_name, "hwmon", 5) == 0) {
            char *num_start = entry->d_name + 5;
            *buff = atoi(num_start);
            break;
        }
    }
    return 1;
}

/*----------------E---READ GPU--------------------------*/

void read_amd_metrics(GPUDevice * device, short hwmon){
    char path[256];
    char hwpath[256];
    snprintf(path, sizeof(path), "%s%s", device->sys_path, DEVICE_PATH);
    get_gpu_busy_percentage(path, &(device->metrics.utilization_gfx_percent));
    get_video_engine_percentage(path, &device->metrics.video_engine_percent);
    get_vram_total(path, &device->metrics.vram_total_mb);
    get_vram_used(path, &device->metrics.vram_used_mb);
    get_sclk_range(path, &device->metrics.core_mhz_range);
    get_mclk_range(path, &device->metrics.memory_mhz_range, &device->metrics.mem_clock_mhz);
    get_driver_name(path, device->metrics.driver_used);
    get_link_seed(path, &device->metrics.link_speed);
    get_link_width(path, &device->metrics.link_width);

    // hwmon
    snprintf(hwpath, sizeof(path), "%s/hwmon/hwmon%hd", path, hwmon);
    get_gpu_watts(hwpath, &device->metrics.power_watts);
    get_gpu_temp(hwpath, &device->metrics.temperature_celsius);
    get_gpu_clock(hwpath, &device->metrics.core_clock_mhz);
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

                res = snprintf(tmp_path, sizeof(tmp_path), "%s/%s/device/hwmon/", DRM_BASE_PATH, entry->d_name);
                get_hw_num(tmp_path, &gpu_device[i].hwwom_num);
                if (res < 0)
                {
                    fprintf(stderr, "Error in card_path formattion %s: %s\n", DRM_BASE_PATH, strerror(errno));
                    return 1;
                }

                res = snprintf(card_path, sizeof(card_path), "%s/%s", DRM_BASE_PATH, entry->d_name);

                if (res < 0)
                {
                    fprintf(stderr, "Error in card_path formattion %s: %s\n", DRM_BASE_PATH, strerror(errno));
                    return 1;
                }

                res = snprintf(tmp_path, sizeof(tmp_path), "%s/device/class", card_path);

                if (res < 0)
                {
                    fprintf(stderr, "Error in class_path formattion %s: %s\n", card_path, strerror(errno));
                    return 1;
                }


                if (is_GPU(tmp_path))
                {
                    strcpy(gpu_device[i].sys_path, card_path);
                    // printf("Path : %s\n", gpu_device[i].sys_path);
                    snprintf(tmp_path, sizeof(tmp_path), "%s/device%s", card_path, VENDOR_PATH);
                    gpu_device[i].vendor_id = get_vendor_id(tmp_path);
                    // printf("VENDOR id : %x\n", gpu_device[i].vendor_id);

                    snprintf(tmp_path, sizeof(tmp_path), "%s/device%s", card_path, DEVICE_PATH);
                    gpu_device[i].device_id = get_vendor_id(tmp_path);
                    // printf("DEVICE id : %x\n", gpu_device[i].device_id);
                    get_dev_name(pacc, gpu_device[i].model_name, sizeof(gpu_device[i].model_name), gpu_device[i].vendor_id, gpu_device[i].device_id);
                    // printf("DEVICE Name : %s\n", gpu_device[i].model_name);
                    if(gpu_device[i].vendor_id == VENDOR_ID_AMD){
                        read_amd_metrics(&gpu_device[i], gpu_device[i].hwwom_num);
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
Todo's
define a futuristics struct
define common uility function
data's like name,gpu size are get from dynamic
speed orient
support intel
support nvidia
support others
*/
/*
* Ref
-  https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/amd/include/kgd_pp_interface.h
-  https://dri.freedesktop.org/docs/drm/gpu/amdgpu/thermal.html#hwmon-interfaces
-  https://gist.github.com/leuc/e45f4dc64dc1db870e4bad1c436228bb
*/