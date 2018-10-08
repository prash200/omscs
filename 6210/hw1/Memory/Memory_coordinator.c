#include <stdio.h>
#include <libvirt/libvirt.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef DEBUG
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

// Represents an array of domain pointers
struct DomainArray
{
    // Pointer to an array of domain pointers
    virDomainPtr *domains;
    // No. of domains in the domain
    int n_domains;
};

// Represents vMemory
struct VMemoryStats
{
    // Domian pointer
    virDomainPtr domain;
    // available memory in vMemory
    unsigned long long available_memory;
    // free memory in vMemory
    unsigned long long free_memory;
};

// Represents a vMemory Array
struct VMemoryStatsArray
{
    // Pointer to an array of vMemorys
    struct VMemoryStats *vMemorys_stats;
    // No. of vMemorys
    int n_vMemorys;
};

// Trace vMemorysStats array
void traceVMemoryStatsArray(struct VMemoryStatsArray* vMemorys_stats)
{
    #ifdef DEBUG
    for (int i = 0 ; i < vMemorys_stats->n_vMemorys ; ++i)
    {
        printf("domain = %p, available_memory = %llu, free_memory = %llu\n", vMemorys_stats->vMemorys_stats[i].domain, vMemorys_stats->vMemorys_stats[i].available_memory, vMemorys_stats->vMemorys_stats[i].free_memory);
    }
    #endif
}

//  Gets an array of active domains
void getActiveDomains(virConnectPtr connection, struct DomainArray *domain_array)
{
    TRACE("getActiveDomains called\n");
    TRACE("connection = %p\n", connection);
    TRACE("domain_array = %p\n", domain_array);

    virDomainPtr *domains = NULL;
    int n_domains = virConnectListAllDomains(connection, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE | VIR_CONNECT_LIST_DOMAINS_RUNNING);

    TRACE("domains = %p\n", domains);
    TRACE("domains = %d\n", n_domains);

    if (n_domains == 0)
    {
        printf("[ERROR] No active domains!\n");
        exit(EXIT_FAILURE);
    }

    domain_array->domains = domains;
    domain_array->n_domains = n_domains;
}

//  Gets vMemory stats
void getVMemoryStats(struct DomainArray *active_domains, struct VMemoryStatsArray *vMemorys_stats)
{
    TRACE("getVMemoryStats called\n");
    TRACE("active_domains = %p\n", active_domains);
    TRACE("vMemorys_stats = %p\n", vMemorys_stats);

    struct VMemoryStats *vMemory_stats = malloc(sizeof(struct VMemoryStats) * active_domains->n_domains);
    TRACE("vMemory_stats = %p\n", vMemory_stats);

    for (int i = 0 ; i < active_domains->n_domains ; ++i)
    {
        TRACE("domain = %p\n", active_domains->domains[i]);

        virDomainMemoryStatStruct memory_stats[VIR_DOMAIN_MEMORY_STAT_NR];
        virDomainMemoryStats(active_domains->domains[i], memory_stats, VIR_DOMAIN_MEMORY_STAT_NR, 0);

        for (int j = 0; j < VIR_DOMAIN_MEMORY_STAT_NR; j++)
        {
            vMemory_stats[i].domain = active_domains->domains[i];

            TRACE("tag = %d, val = %llu\n", memory_stats[j].tag, memory_stats[j].val);

            if(memory_stats[j].tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON)
            {
                vMemory_stats[i].available_memory = memory_stats[j].val;
            }
            else if (memory_stats[j].tag == VIR_DOMAIN_MEMORY_STAT_UNUSED)
            {
                vMemory_stats[i].free_memory = memory_stats[j].val;
            }
        }
    }

    vMemorys_stats->vMemorys_stats = vMemory_stats;
    vMemorys_stats->n_vMemorys = active_domains->n_domains;

    traceVMemoryStatsArray(vMemorys_stats);
}

// balance memory across vMemorys
void balanceMemory(struct VMemoryStatsArray *vMemorys_stats)
{
    TRACE("balanceMemory called\n");
    traceVMemoryStatsArray(vMemorys_stats);

    unsigned long long average_free_memory = 0;
    for (int i = 0 ; i < vMemorys_stats->n_vMemorys ; ++i)
    {
        average_free_memory += vMemorys_stats->vMemorys_stats[i].free_memory;
    }

    average_free_memory /= vMemorys_stats->n_vMemorys;

    TRACE("average_free_memory = %llu\n", average_free_memory);

    for (int i = 0 ; i < vMemorys_stats->n_vMemorys ; ++i)
    {
        unsigned long max_memory = virDomainGetMaxMemory(vMemorys_stats->vMemorys_stats[i].domain);

        TRACE("max_memory = %lu\n", max_memory);

        unsigned long long new_memory = vMemorys_stats->vMemorys_stats[i].available_memory - vMemorys_stats->vMemorys_stats[i].free_memory + average_free_memory;
        new_memory = ((new_memory + 1024) / 1024) * 1024;

        if (new_memory > max_memory)
        {
            new_memory = max_memory;
        }

        TRACE("new_memory = %llu\n", new_memory);

        virDomainSetMemory(vMemorys_stats->vMemorys_stats[i].domain, new_memory);
    }

    traceVMemoryStatsArray(vMemorys_stats);
}

// reclaim vMemorys
void reclaimMemory(struct VMemoryStatsArray *vMemorys_stats)
{
    TRACE("reclaimMemory called\n");
    traceVMemoryStatsArray(vMemorys_stats);

    double min_ratio = 1.0;
    int min_ratio_index = -1;
    for (int i = 0 ; i < vMemorys_stats->n_vMemorys ; ++i)
    {
        double ratio = (double)vMemorys_stats->vMemorys_stats[i].free_memory / (double)vMemorys_stats->vMemorys_stats[i].available_memory;
        TRACE("new ratio = %lf\n", ratio);
        if (ratio < min_ratio)
        {
            min_ratio_index = i;
            min_ratio = ratio;
            TRACE("min ratio = %lf\n", min_ratio);
        }
    }

    if (min_ratio > 0.2 && min_ratio_index != -1)
    {
        unsigned long long average_free_memory = vMemorys_stats->vMemorys_stats[min_ratio_index].available_memory * 0.2;

        TRACE("average_free_memory = %llu\n", average_free_memory);

        for (int i = 0 ; i < vMemorys_stats->n_vMemorys ; ++i)
        {
            unsigned long long new_memory = vMemorys_stats->vMemorys_stats[i].available_memory - vMemorys_stats->vMemorys_stats[i].free_memory + average_free_memory;
            new_memory = ((new_memory + 1024) / 1024) * 1024;

            TRACE("new_memory = %llu\n", new_memory);

            virDomainSetMemory(vMemorys_stats->vMemorys_stats[i].domain, new_memory);
        }
    }

    traceVMemoryStatsArray(vMemorys_stats);
}

// main method
int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("[ERROR] Usage: Memory_coordinator <time interval(s)>.\n");
        return 1;
    }

    virConnectPtr connection = virConnectOpen("qemu:///system");
    while (1)
    {
        struct DomainArray active_domains;
        getActiveDomains(connection, &active_domains);
        if (active_domains.n_domains == 0)
        {
            printf("[ERROR] No active domains to balance. Quitting...\n");
            break;
        }

        struct VMemoryStatsArray vMemorys_stats;

        getVMemoryStats(&active_domains, &vMemorys_stats);
        balanceMemory(&vMemorys_stats);

        vMemorys_stats.n_vMemorys = 0;
        free(vMemorys_stats.vMemorys_stats);

        getVMemoryStats(&active_domains, &vMemorys_stats);
        reclaimMemory(&vMemorys_stats);

        active_domains.n_domains = 0;
        free(active_domains.domains);

        vMemorys_stats.n_vMemorys = 0;
        free(vMemorys_stats.vMemorys_stats);

        sleep(atoi(argv[1]));
    }

    virConnectClose(connection);
    return 0;
}
