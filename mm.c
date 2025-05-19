// Memory manager functions
// J Losh

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include "tm4c123gh6pm.h"
#include "mm.h"
#include "kernel.h"

#define REGIONS 5
#define SUB_REGIONS 40
//Base Addresses
#define START_OS     0x20000000
#define START_ADD4K1 0x20001000  //4k region
#define START_ADD8K1 0x20002000  //8k region
#define START_ADD8K2 0x20004000  //8k region
#define START_ADD4K2 0x20006000  //4k region
#define START_ADD4K3 0x20007000  //4k region

//Top addresses
#define TOP_OS     0x20000FFF
#define TOP_ADD4K1 0x20001FFF
#define TOP_ADD8K1 0x20003FFF
#define TOP_ADD8K2 0x20005FFF
#define TOP_ADD4K2 0x20006FFF
#define TOP_ADD4K3 0x20007FFF

void *NULL_OS = 0x00000000;

typedef struct ownership{
    uint32_t pid;
    uint16_t size;
    uint32_t baseadd;
}owner;

//uint32_t allocate_pid =(uint32_t) 1;

//4k {0,1},8k{2,3},4k{4,5},8k{6,7},4k{8,9}
uint8_t subregions_idx[REGIONS * 2] = {0, 7, 8, 15, 16, 23, 24, 31, 32, 39};
owner Process_MM[16]={{0,0,0},};
uint8_t process=0;
uint8_t Regions_use[SUB_REGIONS]={0,};
uint32_t Address_arr[REGIONS*2]={START_ADD4K1,TOP_ADD4K1,START_ADD8K1,TOP_ADD8K1,START_ADD8K2,TOP_ADD8K2,START_ADD4K2,TOP_ADD4K2,START_ADD4K3,TOP_ADD4K3};
uint16_t Region_subsize[REGIONS] = {512, 1024, 1024, 512, 512};
uint8_t inuse=0;

int SIZE_512 = 512,SIZE_1024 = 1024,SIZE_1536 = 1536;
uint16_t Original_size;

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------
void *Allocation_table(uint16_t size_in_bytes,uint32_t base_address,uint8_t start_sub,uint8_t end_sub,uint8_t Allocation_regions,uint8_t offset)
{
    uint8_t empty_subregions=0,i;
    for(i=start_sub;i<=end_sub;i++)
    {
        if(Regions_use[i]!=0)
        {
            continue;
        }
        else if(Regions_use[i] ==0){
            empty_subregions ++;
        }
    }
    if(empty_subregions>=Allocation_regions)
    {
        uint8_t Allocated =0;
        int track,j;
        for(j=start_sub;j<=end_sub;j++)
        {
              track = j;
              int flag=0;
              while(Allocated < Allocation_regions && track <= end_sub && Regions_use[track]==0)
              {
                track++;
                Allocated++;
               if(Allocation_regions == Allocated)
               {
                 flag=1;
                  break;
               }
              }
              if(flag==1)
              {
              int i;
              for(i=0;i<Allocation_regions;i++)
              {
                track--;
                Regions_use[track] =1;
                empty_subregions --;
              }
              break;
              }
              if(flag==0)
              {
                Allocated =0;
              }
        }
        Process_MM[process].pid =(uint32_t) tcb[process].pid;
//        allocate_pid++;
        Process_MM[process].size = Original_size;
        Process_MM[process].baseadd= (base_address + ((track-offset) * size_in_bytes));
        return (void*) Process_MM[process++].baseadd;
    }
    else{
        return (void*) NULL_OS;
    }
  }
// REQUIRED: add your malloc code here and update the SRD bits for the current thread

void * mallocFromHeap(uint32_t size_in_bytes)
{
    uint8_t lock_allocation = 1;
    void *add;
    if(size_in_bytes <= SIZE_512)
    {
        Original_size = SIZE_512;
        add =  (lock_allocation ==1) ? Allocation_table(SIZE_512,START_ADD4K1,subregions_idx[0],subregions_idx[1]-1,1,0):Allocation_table(SIZE_512,START_ADD4K1,subregions_idx[0],subregions_idx[1],1,0);
        if(add!=NULL_OS)
        {
            return add;
        }
        add =  (lock_allocation ==1) ? Allocation_table(SIZE_512,START_ADD4K2,subregions_idx[6]+1,subregions_idx[7],1,24):Allocation_table(SIZE_512,START_ADD4K2,subregions_idx[6],subregions_idx[7],1,24);
        if(add!=NULL_OS)
        {
            return add;
        }
        add =  (lock_allocation ==1) ? Allocation_table(SIZE_512,START_ADD4K3,subregions_idx[8],subregions_idx[9],1,32):Allocation_table(SIZE_512,START_ADD4K3,subregions_idx[8],subregions_idx[9],1,32);
        if(add!=NULL_OS)
        {
            return add;
        }
    }
    else if(size_in_bytes > SIZE_512 && size_in_bytes <= SIZE_1024)
    {
        Original_size = SIZE_1024;
        add = (lock_allocation == 1) ? Allocation_table(SIZE_1024,START_ADD8K1,subregions_idx[2]+1,subregions_idx[3],1,8) : Allocation_table(SIZE_1024,START_ADD8K1,subregions_idx[2],subregions_idx[3],1,8);
        if(add != NULL_OS)
        {
            return add;
        }
        add = (lock_allocation == 1) ? Allocation_table(SIZE_1024,START_ADD8K2,subregions_idx[4],subregions_idx[5]-1,1,16) :  Allocation_table(SIZE_1024,START_ADD8K2,subregions_idx[4],subregions_idx[5],1,16);
        if(add != NULL_OS)
        {
            return add;
        }
        uint8_t Allocation = (size_in_bytes % SIZE_512 == 0) ? (size_in_bytes/SIZE_512) : ((size_in_bytes + SIZE_512 -1 )/SIZE_512);
        add =  (lock_allocation ==1) ? Allocation_table(SIZE_512,START_ADD4K1,subregions_idx[0],subregions_idx[1]-1,Allocation,0):Allocation_table(SIZE_512,START_ADD4K1,subregions_idx[0],subregions_idx[1],Allocation,0);
        if(add!=NULL_OS)
        {
            return add;
        }
        add =  (lock_allocation ==1) ? Allocation_table(SIZE_512,START_ADD4K2,subregions_idx[6]+1,subregions_idx[7],Allocation,24):Allocation_table(SIZE_512,START_ADD4K2,subregions_idx[6],subregions_idx[7],Allocation,24);
        if(add!=NULL_OS)
        {
            return add;
        }
        add =  (lock_allocation ==1) ? Allocation_table(SIZE_512,START_ADD4K3,subregions_idx[8],subregions_idx[9],Allocation,32):Allocation_table(SIZE_512,START_ADD4K3,subregions_idx[8],subregions_idx[9],Allocation,32);
        if(add!=NULL_OS)
        {
            return add;
        }

    }
    else if(size_in_bytes>SIZE_1024 && size_in_bytes<= SIZE_1536)
    {
        Original_size = SIZE_1536; //is this the only case??
        add = Allocation_table(SIZE_512,START_ADD4K1,subregions_idx[1],subregions_idx[2],2,0);
        if(add != NULL_OS)
        {
            return add;
        }
        add = Allocation_table(SIZE_1024,START_ADD8K2,subregions_idx[5],subregions_idx[6],2,16);
        if(add !=NULL_OS)
        {
            return add;
        }
    }
    else if(size_in_bytes > SIZE_1536)
    {
        Original_size = size_in_bytes;
        uint8_t Allocation = (size_in_bytes % SIZE_1024 == 0) ? (size_in_bytes/SIZE_1024) : ((size_in_bytes + SIZE_1024 -1 )/SIZE_1024);
        add = (lock_allocation == 1) ? Allocation_table(SIZE_1024,START_ADD8K1,subregions_idx[2]+1,subregions_idx[3],Allocation,8) : Allocation_table(SIZE_1024,START_ADD8K1,subregions_idx[2],subregions_idx[3],Allocation,8);
        if(add != NULL_OS)
        {
            return add;
        }
        add = (lock_allocation == 1) ? Allocation_table(SIZE_1024,START_ADD8K2,subregions_idx[4],subregions_idx[5]-1,Allocation,16) :  Allocation_table(SIZE_1024,START_ADD8K2,subregions_idx[4],subregions_idx[5],Allocation,16);
        if(add != NULL_OS)
        {
            return add;
        }
    }
    return NULL_OS;
}

// REQUIRED: add your free code here and update the SRD bits for the current thread
//uint32_t allow_pid = (uint32_t) 1;

void freeToHeap(void *pMemory)
{
    int idx,flag = 0;
    uint16_t size_in_bytes;
    uint32_t pmemory = (uint32_t) pMemory;
    for(idx=0;idx<12;idx++)
    {
        if(Process_MM[idx].baseadd == pmemory)
        {
            uint32_t task_pid = (uint32_t) tcb[idx].pid;
            if(Process_MM[idx].pid == task_pid)
            {
                size_in_bytes = Process_MM[idx].size;
                flag=1;
                break;
            }
        }
    }
    if(flag == 1)
    {
        uint32_t store_add = pmemory;
        uint32_t point_Address = pmemory;
        int idx2;
        for (idx2 = 0; idx2 <= (REGIONS * 2) - 2; idx2 += 2) {
            if (point_Address >= Address_arr[idx2] && point_Address <= Address_arr[idx2 + 1]) {
                while (point_Address < (store_add + size_in_bytes)) {
                    if (point_Address >= Address_arr[idx2] && point_Address <= Address_arr[idx2 + 1]) {
                        uint32_t offset = point_Address - Address_arr[idx2];
                        uint8_t subregion = (offset / (Region_subsize[(idx2 / 2)]));
                        uint8_t check = (subregion) + (subregions_idx[idx2]);
                        Regions_use[check]=0;
                        point_Address += Region_subsize[(idx2 / 2)];
                    }
                    else break;
                }
            }
        }
    }
    else if(flag == 0) return;
}

// REQUIRED: include your solution from the mini project
void SetBackground_Rules(void)
{
    NVIC_MPU_NUMBER_R = 0x00000000;
    NVIC_MPU_BASE_R = 0x00000000;
    //Enabling all 32 bit regions with Shareable and Cacheable attributes, with read write access for privileged and unprivileged permissions. //AP: 011 TEX: 000
    NVIC_MPU_ATTR_R |= NVIC_MPU_ATTR_XN | 0x03000000 | 0x00000000 | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | NVIC_MPU_ATTR_BUFFRABLE | (0x1F<<1) | NVIC_MPU_ATTR_ENABLE; //4gb
}

void allowFlashAccess(void)
{
    NVIC_MPU_NUMBER_R = 0x00000001;
    NVIC_MPU_BASE_R = 0x00000000; //256kib flash
    //Enabling the regions of full 32kib with Cacheable attributes, with read write access for privileged and unprivileged permissions. //AP: 011 TEX: 000
    NVIC_MPU_ATTR_R |= 0x03000000| 0x00000000 | NVIC_MPU_ATTR_CACHEABLE | (0x11<<1) | NVIC_MPU_ATTR_ENABLE;
}

void setupSramAccess(void)
{
    NVIC_MPU_NUMBER_R = 0x00000002;
    NVIC_MPU_BASE_R  = START_OS;
    NVIC_MPU_ATTR_R |=  0x01000000 | 0x00000000 | NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | 0xB << 1 | NVIC_MPU_ATTR_ENABLE;

    NVIC_MPU_NUMBER_R = 0x00000003;
    NVIC_MPU_BASE_R  =  START_ADD4K1;
    NVIC_MPU_ATTR_R |=  0x01000000 | 0x00000000| NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | 0xB << 1 | NVIC_MPU_ATTR_ENABLE;

    NVIC_MPU_NUMBER_R = 0x00000004;
    NVIC_MPU_BASE_R  =  START_ADD8K1;
    NVIC_MPU_ATTR_R |=  0x01000000 | 0x00000000| NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | 0xC << 1 | NVIC_MPU_ATTR_ENABLE;

    NVIC_MPU_NUMBER_R = 0x00000005;
    NVIC_MPU_BASE_R  =  START_ADD8K2;
    NVIC_MPU_ATTR_R |=  0x01000000 |0x00000000| NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | 0xC << 1 | NVIC_MPU_ATTR_ENABLE;

    NVIC_MPU_NUMBER_R = 0x00000006;
    NVIC_MPU_BASE_R  =  START_ADD4K2;
    NVIC_MPU_ATTR_R |=  0x01000000  | 0x00000000| NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | 0xB << 1 | NVIC_MPU_ATTR_ENABLE;

    NVIC_MPU_NUMBER_R = 0x00000007;
    NVIC_MPU_BASE_R  =  START_ADD4K3;
    NVIC_MPU_ATTR_R |=  0x01000000 | 0x00000000|  NVIC_MPU_ATTR_SHAREABLE | NVIC_MPU_ATTR_CACHEABLE | 0xB << 1 | NVIC_MPU_ATTR_ENABLE;

}

uint64_t createNoSramAccessMask(void)
{
    return 0x0000000000000000;
}

void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes)
{
    if (size_in_bytes > 32768) return;

    uint32_t baseadd = (uint32_t) baseAdd;
    uint32_t store_add = baseadd;
    uint32_t point_Address = baseadd;
    int idx2;
    for (idx2 = 0; idx2 <= (REGIONS * 2) - 2; idx2 += 2) {
        if (point_Address >= Address_arr[idx2] && point_Address <= Address_arr[idx2 + 1]) {
            while (point_Address < (store_add + size_in_bytes)) {
                if (point_Address >= Address_arr[idx2] && point_Address <= Address_arr[idx2 + 1]) {
                    uint32_t offset = point_Address - Address_arr[idx2];
                    uint8_t subregion = (offset / (Region_subsize[(idx2 / 2)]));
                    uint8_t check = (subregion) + (subregions_idx[idx2]);
                    *srdBitMask |= (1ULL << check);
                    point_Address += Region_subsize[(idx2 / 2)];
                } else break;
            }
        }
    }
}

void applySramAccessMask(uint64_t srdBitMask)
{
    NVIC_MPU_NUMBER_R = 0x00000003;
    NVIC_MPU_ATTR_R &= ~0xFF00;
    NVIC_MPU_ATTR_R |=(uint32_t)  ((srdBitMask >>0 ) & 0xFF) <<8;

    NVIC_MPU_NUMBER_R = 0x00000004;
    NVIC_MPU_ATTR_R &= ~0xFF00;
    NVIC_MPU_ATTR_R |=(uint32_t)  ((srdBitMask >>8 ) & 0xFF) <<8;

    NVIC_MPU_NUMBER_R = 0x00000005;
    NVIC_MPU_ATTR_R &= ~0xFF00;
    NVIC_MPU_ATTR_R |= (uint32_t) ((srdBitMask >>16 ) & 0xFF) <<8;

    NVIC_MPU_NUMBER_R = 0x00000006;
    NVIC_MPU_ATTR_R &= ~0xFF00;
    NVIC_MPU_ATTR_R |= (uint32_t) ((srdBitMask >>24 ) & 0xFF) <<8;

    NVIC_MPU_NUMBER_R = 0x00000007;
    NVIC_MPU_ATTR_R &= ~0xFF00;
    NVIC_MPU_ATTR_R |= (uint32_t) ((srdBitMask >>32 ) & 0xFF) <<8;

}


