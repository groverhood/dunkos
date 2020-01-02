#include <stdint.h>
#include <synch.h>
#include <string.h>
#include <asm.h>
#include <stdext.h>
#include <algo.h>
#include <util/hashtable.h>
#include <util/debug.h>
#include <pci.h>

#define PACKED __attribute__((packed))
#define PCI_MAX_DEVICES 32

struct PACKED rsdp_descriptor {
    char sig[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdtaddr;

    uint32_t length;
    uint64_t xsdtaddr;
    uint8_t extchecksum;
    uint8_t reserved[3];
};

struct PACKED cfg_space_allocation {
    uint64_t ecfgbase;
    uint16_t pciseggnum;
    uint8_t startpcibusn;
    uint8_t endpcibusn;
    uint32_t resvd;
};

struct PACKED mcfg_table {
    char sig[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    uint64_t oemtblid;
    uint32_t oemrevision;
    uint32_t creatorid;
    uint32_t creatorrevision;
    uint64_t resvd;
};

static struct pci_device device_table[PCI_DCLASS_UNCLASSIFIED];
static uint16_t pci_config_word(struct cfg_space_allocation *cfg, uint8_t bus, uint8_t device, uint8_t func, uint8_t ofs);
static void enumerate_pci_devices(void);

void init_pci(void)
{
    enumerate_pci_devices();
}

struct pci_device *get_device(enum pci_device_class class)
{
    return (device_table + class);
}

static struct rsdp_descriptor *find_rsdp(void);
static void enumerate_device(struct cfg_space_allocation *cfg);
static void *find_sdt(void *xsdt, const char *name);

static void enumerate_pci_devices(void)
{
    struct rsdp_descriptor *rsdp = find_rsdp();

    assert(rsdp != NULL);
    assert(memsum(rsdp, offsetof(struct rsdp_descriptor, length)) & 1 == 0);
    assert(memsum(&rsdp->length, sizeof *rsdp - offsetof(struct rsdp_descriptor, length)) & 1 == 0);

    struct mcfg_table *mcfg = find_sdt((void *)rsdp->xsdtaddr, "MCFG");

    assert(mcfg != NULL);
    assert(memsum(mcfg, mcfg->length) & 0xFF == 0);

    struct cfg_space_allocation *table = (struct cfg_space_allocation *)(mcfg + 1);
    size_t i, length = (mcfg->length - sizeof *mcfg) / sizeof(struct cfg_space_allocation);
    for (i = 0; i < length; ++i) {
        enumerate_device(table + i);
    }
}

static void enumerate_device(struct cfg_space_allocation *cfg)
{
    int bus, dev, end = cfg->endpcibusn;
    for (bus = cfg->startpcibusn; bus < end; ++bus) {
        for (dev = 0; dev < PCI_MAX_DEVICES; ++dev) {
            if (pci_config_word(cfg, bus, dev, 0, 0) == 0xFFFF) {
                continue;
            }

            uint16_t classinfo = pci_config_word(cfg, bus, dev, 0, 5);
            enum pci_device_class class = (classinfo >> 8);
            int subclass = (classinfo & 0xFF);

            struct pci_device *device = (device_table + class);
            device->bus = bus;
            device->class = class;
            device->subclass = subclass;
            device->config = cfg;
            device->id = pci_config_word(cfg, bus, dev, 0, 1);
            device->progif = (uint8_t)(pci_config_word(cfg, bus, dev, 0, 4) >> 8);
        }
    }
}

static uint16_t pci_config_word(struct cfg_space_allocation *cfg, uint8_t bus, uint8_t device, uint8_t func, uint8_t ofs)
{
    uint32_t config_data;
    uint32_t lbus = bus;
    uint32_t ldev = device;
    uint32_t lfunc = func;

    uint16_t *cfgwordptr = (uint16_t *)(
          (cfg->ecfgbase)
        + (((lbus - cfg->startpcibusn) << 20) 
        | (ldev << 15) 
        | (lfunc << 12))
    );

    return cfgwordptr[ofs];
}

static struct rsdp_descriptor *find_rsdp(void)
{
    const size_t inc = 16 * div_rnd_up(sizeof(struct rsdp_descriptor), 16);

    uint8_t *ptr = (uint8_t *)(0x000E0000);
    while (ptr != (uint8_t *)(0x000FFFFF)) {
        if (!strncmp((const char *)ptr, "RSD PTR ", 8)) {
            break;
        }
        ptr += inc;
    }

    return (ptr == (uint8_t *)(0x000FFFFF)) ? (NULL) : ((struct rsdp_descriptor *)ptr);
}

struct PACKED sdt_header {
    char sig[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemid[6];
    char oemtblid[8];
    uint32_t oemrevision;
    uint32_t creatorid;
    uint32_t creatorrevision;
};

static void *find_sdt(void *xsdt_, const char *name) {
    struct sdt_header *xsdt = xsdt_;
    assert(memsum(xsdt, xsdt->length) & 0xFF == 0);

    char namecmpstr[4];
    memcpy(namecmpstr, name, min(strlen(name), 4));

    void *sdt = NULL;
    struct sdt_header *tbl = xsdt + 1;
    size_t i, len = (xsdt->length - sizeof *xsdt) / (sizeof *xsdt);
    for (i = 0; i < len; ++i) {
        if (!strncmp(namecmpstr, tbl[i].sig, 4)) {
            sdt = (tbl + i);
            break;
        }
    }

    return sdt;
}
