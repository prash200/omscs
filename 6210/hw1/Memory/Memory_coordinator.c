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
    // used memory in vMemory
    unsigned long long used_memory;
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
        printf("domain = %p, used_memory = %llu, free_memory = %llu\n", vMemorys_stats->vMemorys_stats[i].domain, vMemorys_stats->vMemorys_stats[i].used_memory, vMemorys_stats->vMemorys_stats[i].free_memory);
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

            if(memory_stats[j].tag == VIR_DOMAIN_MEMORY_STAT_RSS)
            {
                vMemory_stats[i].used_memory = memory_stats[j].val;
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

// balance load across vMemorys
void balanceLoad(struct VMemoryStatsArray *vMemorys_stats)
{
    TRACE("balanceLoad called\n");
    traceVMemoryStatsArray(vMemorys_stats);

    unsigned long long avgFreeMemory = 0;
    for (int i = 0 ; i < vMemorys_stats->n_vMemorys ; ++i)
    {
        avgFreeMemory += vMemorys_stats->vMemorys_stats[i].free_memory;
    }

    avgFreeMemory /= vMemorys_stats->n_vMemorys;

    TRACE("avgFreeMemory = %llu\n", avgFreeMemory);

    for (int i = 0 ; i < vMemorys_stats->n_vMemorys ; ++i)
    {
        TRACE("new_memory = %llu\n", vMemorys_stats->vMemorys_stats[i].used_memory + avgFreeMemory);
        TRACE("max_memory = %llu\n", virDomainGetMaxMemory(vMemorys_stats->vMemorys_stats[i].domain));

        virDomainSetMemory(vMemorys_stats->vMemorys_stats[i].domain, vMemorys_stats->vMemorys_stats[i].used_memory + avgFreeMemory);
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

        balanceLoad(&vMemorys_stats);

        active_domains.n_domains = 0;
        free(active_domains.domains);

        vMemorys_stats.n_vMemorys = 0;
        free(vMemorys_stats.vMemorys_stats);

        sleep(atoi(argv[1]));
    }

    virConnectClose(connection);
    return 0;
}
