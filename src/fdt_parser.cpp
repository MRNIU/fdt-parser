
// This file is a part of MRNIU/fdt-parser
// (https://github.com/MRNIU/fdt-parser).
// Based on https://github.com/brenns10/sos
// fdt_parser.cpp for MRNIU/fdt-parser.

#include "stdint.h"
#include "stdio.h"
#include "assert.h"
#include "string.h"
#include "fdt_parser.h"
#include "machine/endian.h"

// 对齐 向上取整
static inline uintptr_t ALIGN(uintptr_t _x, size_t _align) {
    return ((_x + _align - 1) & (~(_align - 1)));
}

fdt_parser::dtb_info_t fdt_parser::dtb_info;
// 所有节点
fdt_parser::node_t fdt_parser::nodes[MAX_NODES];
// 节点数
size_t fdt_parser::node_t::count = 0;
// 所有 phandle
fdt_parser::phandle_map_t fdt_parser::phandle_map[MAX_NODES];
// phandle 数
size_t fdt_parser::phandle_map_t::count = 0;

bool fdt_parser::path_t::operator==(const fdt_parser::path_t *_path) {
    if (len != _path->len) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        if (strcmp(path[i], _path->path[i]) != 0) {
            return false;
        }
    }
    return true;
}

bool fdt_parser::node_t::find(const char *_prop_name, const char *_val) {
    for (size_t i = 0; i < prop_count; i++) {
        // 匹配属性
        if (strcmp(props[i].name, _prop_name) == 0) {
            // 匹配值
            if (strcmp((char *)props[i].addr, _val) == 0) {
                return true;
            }
        }
    }
    return false;
}

fdt_parser::node_t *fdt_parser::get_phandle(uint32_t _phandle) {
    // 在 phandle_map 中寻找对应的节点
    for (size_t i = 0; i < phandle_map[0].count; i++) {
        if (phandle_map[i].phandle == _phandle) {
            return phandle_map[i].node;
        }
    }
    return nullptr;
}

fdt_parser::dt_fmt_t fdt_parser::get_fmt(const char *_prop_name) {
    // 默认为 FMT_UNKNOWN
    dt_fmt_t res = FMT_UNKNOWN;
    for (size_t i = 0; i < sizeof(props) / sizeof(dt_prop_fmt_t); i++) {
        // 找到了则更新
        if (strcmp(_prop_name, props[i].prop_name) == 0) {
            res = props[i].fmt;
        }
    }
    return res;
}

void fdt_parser::print_attr_propenc(const iter_data_t *_iter, size_t *_cells,
                                    size_t _len) {
    // 字节总数
    uint32_t entry_size = 0;
    // 属性长度
    uint32_t remain = _iter->prop_len;
    // 属性数据
    uint32_t *reg = _iter->prop_addr;
    printf("%s: ", _iter->prop_name);

    // 计算
    for (size_t i = 0; i < _len; i++) {
        entry_size += 4 * _cells[i];
    }

    printf("(len=%u/%u) ", _iter->prop_len, entry_size);

    // 理论上应该是可以整除的，如果不行说明有问题
    assert(_iter->prop_len % entry_size == 0);

    // 数据长度大于 0
    while (remain > 0) {
        std::cout << "<";
        for (size_t i = 0; i < _len; i++) {
            // 直接输出
            for (size_t j = 0; j < _cells[i]; j++) {
                printf("0x%X ", ntohl(*reg));
                // 下一个 cell
                reg++;
                // 减 4，即一个 cell 大小
                remain -= 4;
            }

            if (i != _len - 1) {
                std::cout << "| ";
            }
        }
        // \b 删除最后一个 "| " 中的空格
        std::cout << "\b>";
    }
    return;
}

void fdt_parser::fill_resource(resource_t *_resource, const node_t *_node,
                               const prop_t *_prop) {
    // 内存类型
    if (_resource->type == resource_t::MEM) {
        // 根据 address_cells 与 size_cells 填充
        // resource 一般来说两者是相等的
        if (_node->parent->address_cells == 1) {
            assert(_node->parent->size_cells == 1);
            _resource->mem.addr = ntohl(((uint32_t *)_prop->addr)[0]);
            _resource->mem.len  = ntohl(((uint32_t *)_prop->addr)[1]);
        }
        else if (_node->parent->address_cells == 2) {
            assert(_node->parent->size_cells == 2);
            _resource->mem.addr = ntohl(((uint32_t *)_prop->addr)[0]);
            _resource->mem.addr += ntohl(((uint32_t *)_prop->addr)[1]);
            _resource->mem.len = ntohl(((uint32_t *)_prop->addr)[2]);
            _resource->mem.len += ntohl(((uint32_t *)_prop->addr)[3]);
        }
        else {
            assert(0);
        }
    }
    return;
}

fdt_parser::node_t *fdt_parser::find_node(const char *_prop_name,
                                          const char *_val) {
    node_t *res = nullptr;
    // 遍历 nodes
    for (size_t i = 0; i < nodes[0].count; i++) {
        // 如果 nodes[i] 中有属性/值对符合要求
        if (nodes[i].find(_prop_name, _val) == true) {
            // 设置返回值
            res = &nodes[i];
        }
    }
    return res;
}

void fdt_parser::dtb_mem_reserved(void) {
    fdt_reserve_entry_t *entry = dtb_info.reserved;
    if (entry->addr_le || entry->size_le) {
        // 目前没有考虑这种情况，先报错
        assert(0);
    }
    return;
}

void fdt_parser::dtb_iter(uint8_t _cb_flags,
                          bool (*_cb)(const iter_data_t *, void *),
                          void *_data) {
    // 迭代变量
    iter_data_t iter;
    // 路径深度
    iter.path.len = 0;
    // 数据地址
    iter.addr = (uint32_t *)dtb_info.data;
    // 节点索引
    iter.nodes_idx = 0;
    // 开始 flag
    bool begin = true;

    while (1) {
        //
        iter.type = ntohl(iter.addr[0]);
        switch (iter.type) {
            case FDT_NOP: {
                // 跳过 type
                iter.addr++;
                break;
            }
            case FDT_BEGIN_NODE: {
                // 第 len 深底的名称
                iter.path.path[iter.path.len] = (char *)(iter.addr + 1);
                // 深度+1
                iter.path.len++;
                iter.nodes_idx = begin ? 0 : (iter.nodes_idx + 1);
                begin          = false;
                if (_cb_flags & DT_ITER_BEGIN_NODE) {
                    if (_cb(&iter, _data)) {
                        return;
                    }
                }
                // 跳过 type
                iter.addr++;
                // 跳过 name
                iter.addr += ALIGN(strlen((char *)iter.addr) + 1, 4) / 4;
                break;
            }
            case FDT_END_NODE: {
                if (_cb_flags & DT_ITER_END_NODE) {
                    if (_cb(&iter, _data)) {
                        return;
                    }
                }
                // 这一级结束了，所以 -1
                iter.path.len--;
                // 跳过 type
                iter.addr++;
                break;
            }
            case FDT_PROP: {
                iter.prop_len  = ntohl(iter.addr[1]);
                iter.prop_name = (char *)(dtb_info.str + ntohl(iter.addr[2]));
                iter.prop_addr = iter.addr + 3;
                if (_cb_flags & DT_ITER_PROP) {
                    if (_cb(&iter, _data)) {
                        return;
                    }
                }
                iter.prop_name = nullptr;
                iter.prop_addr = nullptr;
                // 跳过 type
                iter.addr++;
                // 跳过 len
                iter.addr++;
                // 跳过 nameoff
                iter.addr++;
                // 跳过 data，并进行对齐
                iter.addr += ALIGN(iter.prop_len, 4) / 4;
                iter.prop_len = 0;
                break;
            }
            case FDT_END: {
                return;
            }
            default: {
                printf("unrecognized token 0x%X\n", iter.type);
                return;
            }
        }
    }
    return;
}

/*
 * This callback constructs tracking information about each node.
 */
bool fdt_parser::dtb_init_cb(const iter_data_t *_iter, void *) {
    // 索引
    size_t idx = _iter->nodes_idx;
    // 根据类型
    switch (_iter->type) {
        // 开始
        case FDT_BEGIN_NODE: {
            // 设置节点基本信息
            nodes[idx].path  = _iter->path;
            nodes[idx].addr  = _iter->addr;
            nodes[idx].depth = _iter->path.len;
            // 设置默认值
            nodes[idx].address_cells    = 2;
            nodes[idx].size_cells       = 2;
            nodes[idx].interrupt_cells  = 0;
            nodes[idx].phandle          = 0;
            nodes[idx].interrupt_parent = nullptr;
            // 设置父节点
            // 如果不是根节点
            if (idx != 0) {
                size_t i = idx - 1;
                while (nodes[i].depth != nodes[idx].depth - 1) {
                    i--;
                }
                nodes[idx].parent = &nodes[i];
            }
            // 索引为 0 说明是根节点
            else {
                // 根节点的父节点为空
                nodes[idx].parent = nullptr;
            }
            break;
        }
        case FDT_PROP: {
            // 获取 cells 信息
            if (strcmp(_iter->prop_name, "#address-cells") == 0) {
                nodes[idx].address_cells = ntohl(_iter->addr[3]);
            }
            else if (strcmp(_iter->prop_name, "#size-cells") == 0) {
                nodes[idx].size_cells = ntohl(_iter->addr[3]);
            }
            else if (strcmp(_iter->prop_name, "#interrupt-cells") == 0) {
                nodes[idx].interrupt_cells = ntohl(_iter->addr[3]);
            }
            // phandle 信息
            else if (strcmp(_iter->prop_name, "phandle") == 0) {
                nodes[idx].phandle = ntohl(_iter->addr[3]);
                // 更新 phandle_map
                phandle_map[phandle_map[0].count].phandle = nodes[idx].phandle;
                phandle_map[phandle_map[0].count].node    = &nodes[idx];
                phandle_map[0].count++;
            }
            // 添加属性
            nodes[idx].props[nodes[idx].prop_count].name = _iter->prop_name;
            nodes[idx].props[nodes[idx].prop_count].addr =
                (uintptr_t)(_iter->addr + 3);
            nodes[idx].props[nodes[idx].prop_count].len = ntohl(_iter->addr[1]);
            nodes[idx].prop_count++;
            break;
        }
        case FDT_END_NODE: {
            // 节点数+1
            nodes[0].count = idx + 1;
            break;
        }
    }
    // 返回 false 表示需要迭代全部节点
    return false;
}

bool fdt_parser::dtb_init_interrupt_cb(const iter_data_t *_iter, void *) {
    uint8_t  idx = _iter->nodes_idx;
    uint32_t phandle;
    node_t * parent;
    // 设置中断父节点
    if (strcmp(_iter->prop_name, "interrupt-parent") == 0) {
        phandle = ntohl(_iter->addr[3]);
        parent  = get_phandle(phandle);
        // 没有找到则报错
        assert(parent != nullptr);
        nodes[idx].interrupt_parent = parent;
    }
    // 返回 false 表示需要迭代全部节点
    return false;
}

bool fdt_parser::fdt_parser_init(void *_addr) {
    // 头信息
    dtb_info.header = (fdt_header_t *)_addr;
    // 魔数
    assert(ntohl(dtb_info.header->magic) == FDT_MAGIC);
    // 版本
    assert(ntohl(dtb_info.header->version) == FDT_VERSION);
    // 设置大小
    dtb_info.total_size = ntohl(dtb_info.header->totalsize);
    // 内存保留区
    dtb_info.reserved =
        (fdt_reserve_entry_t *)((uintptr_t)_addr +
                                ntohl(dtb_info.header->off_mem_rsvmap));
    // 数据区
    dtb_info.data = ((uintptr_t)_addr) + ntohl(dtb_info.header->off_dt_struct);
    // 字符区
    dtb_info.str = ((uintptr_t)_addr) + ntohl(dtb_info.header->off_dt_strings);
    // 检查保留内存
    dtb_mem_reserved();
    // 初始化 map
    bzero(nodes, sizeof(nodes));
    bzero(phandle_map, sizeof(phandle_map));
    // 初始化节点的基本信息
    dtb_iter(DT_ITER_BEGIN_NODE | DT_ITER_END_NODE | DT_ITER_PROP, dtb_init_cb,
             nullptr);
    // 中断信息初始化，因为需要查找 phandle，所以在基本信息初始化完成后进行
    dtb_iter(DT_ITER_PROP, dtb_init_interrupt_cb, nullptr);
// #define DEBUG
#ifdef DEBUG
    // 输出所有信息
    for (size_t i = 0; i < nodes[0].count; i++) {
        std::cout << nodes[i].path << ": " << std::endl;
        for (size_t j = 0; j < nodes[i].prop_count; j++) {
            printf("%s: ", nodes[i].props[j].name);
            for (size_t k = 0; k < nodes[i].props[j].len / 4; k++) {
                printf("0x%X ", ntohl(((uint32_t *)nodes[i].props[j].addr)[k]));
            }
            printf("\n");
        }
    }
#undef DEBUG
#endif
    std::cout << "fdt init." << std::endl;
    return true;
}

resource_t fdt_parser::find(const char *_prop_name, const char *_val) {
    resource_t resource;
    // 找到节点
    auto node = find_node(_prop_name, _val);
    // 根据类型不同进行相应设置
    if (strcmp(_val, "memory") == 0) {
        // 设置 resource 基本信息
        resource.name = (char *)"memory";
        resource.type = resource_t::MEM;
    }
    // 找到 reg
    for (size_t i = 0; i < node->prop_count; i++) {
        if (strcmp(node->props[i].name, "reg") == 0) {
            // 填充数据
            fill_resource(&resource, node, &node->props[i]);
            break;
        }
    }
    return resource;
}

std::ostream &operator<<(std::ostream &                 _os,
                         const fdt_parser::iter_data_t &_iter) {
    // 输出路径
    _os << _iter.path << ": ";
    // 根据属性类型输出
    switch (fdt_parser::get_fmt(_iter.prop_name)) {
        // 未知
        case fdt_parser::FMT_UNKNOWN: {
            printf("%s: (unknown format, len=0x%X)", _iter.prop_name,
                   _iter.prop_len);
            break;
        }
        // 空
        case fdt_parser::FMT_EMPTY: {
            _os << _iter.prop_name << ": (empty)";
            break;
        }
        // 32 位整数
        case fdt_parser::FMT_U32: {
            printf("%s: 0x%X", _iter.prop_name,
                   ntohl(*(uint32_t *)_iter.prop_addr));
            break;
        }
        // 64 位整数
        case fdt_parser::FMT_U64: {
            printf("%s: %llu", _iter.prop_name,
                   ntohll(*(uint64_t *)_iter.prop_addr));
            break;
        }
        // 字符串
        case fdt_parser::FMT_STRING: {
            _os << _iter.prop_name << ": " << (char *)_iter.prop_addr;
            break;
        }
        // phandle
        case fdt_parser::FMT_PHANDLE: {
            uint32_t            phandle = ntohl(_iter.addr[3]);
            fdt_parser::node_t *ref     = fdt_parser::get_phandle(phandle);
            if (ref != nullptr) {
                printf("%s: <phandle &%s>", _iter.prop_name, ref->path.path[0]);
            }
            else {
                printf("%s: <phandle 0x%X>", _iter.prop_name, phandle);
            }
            break;
        }
        // 字符串列表
        case fdt_parser::FMT_STRINGLIST: {
            size_t len = 0;
            char * str = (char *)_iter.prop_addr;
            _os << _iter.prop_name << ": [";
            while (len < _iter.prop_len) {
                // 用 "" 分隔
                _os << "\"" << str << "\"";
                len += strlen(str) + 1;
                str = (char *)((uint8_t *)_iter.prop_addr + len);
            }
            _os << "]";
            break;
        }
        // reg，不定长的 32 位数据
        case fdt_parser::FMT_REG: {
            // 获取节点索引
            uint8_t idx = _iter.nodes_idx;
            // 全部 cells 大小
            // devicetree-specification-v0.3.pdf#2.3.6
            size_t cells[] = {
                fdt_parser::nodes[idx].parent->address_cells,
                fdt_parser::nodes[idx].parent->size_cells,
            };
            // 调用辅助函数进行输出
            fdt_parser::print_attr_propenc(&_iter, cells,
                                           sizeof(cells) / sizeof(size_t));
            break;
        }
        case fdt_parser::FMT_RANGES: {
            // 获取节点索引
            uint8_t idx = _iter.nodes_idx;
            // 全部 cells 大小
            // devicetree-specification-v0.3.pdf#2.3.8
            size_t cells[] = {
                fdt_parser::nodes[idx].address_cells,
                fdt_parser::nodes[idx].parent->address_cells,
                fdt_parser::nodes[idx].size_cells,
            };
            // 调用辅助函数进行输出
            fdt_parser::print_attr_propenc(&_iter, cells,
                                           sizeof(cells) / sizeof(size_t));
            break;
        }
        default: {
            assert(0);
            break;
        }
    }
    return _os;
}

std::ostream &operator<<(std::ostream &_os, const fdt_parser::path_t &_path) {
    if (_path.len == 1) {
        _os << "/";
    }
    else {
        for (size_t i = 1; i < _path.len; i++) {
            _os << "/";
            _os << _path.path[i];
        }
    }
    return _os;
}
