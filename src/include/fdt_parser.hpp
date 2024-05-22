
// This file is a part of MRNIU/fdt-parser
// (https://github.com/MRNIU/fdt-parser).
// Based on https://github.com/brenns10/sos
// fdt_parser.h for MRNIU/fdt-parser.

#ifndef FDT_PARSER_SRC_INCLUDE_FDT_PARSER_H
#define FDT_PARSER_SRC_INCLUDE_FDT_PARSER_H

#include "cstdint"
#include "functional"
#include "iostream"

// See devicetree-specification-v0.3.pdf
// https://e-mailky.github.io/2016-12-06-dts-introduce
// https://e-mailky.github.io/2019-01-14-dts-1
// https://e-mailky.github.io/2019-01-14-dts-2
// https://e-mailky.github.io/2019-01-14-dts-3

namespace FDT_PARSER {

int fdt_parser_printf(const char* format, ...);

// 对齐 向上取整
static inline uintptr_t ALIGN(uintptr_t _x, size_t _align) {
  return ((_x + _align - 1) & (~(_align - 1)));
}

/**
 * @brief 用于表示一种资源
 */
struct resource_t {
  /// 资源类型
  enum : uint8_t {
    /// 内存
    MEM = 1 << 0,
    /// 中断号
    INTR_NO = 1 << 1,
  };

  uint8_t type;
  /// 资源名称
  char* name;

  /// 内存信息
  struct {
    uintptr_t addr;
    size_t len;
  } mem;

  /// 中断号
  uint8_t intr_no;

  resource_t(void) : type(0), name(nullptr) {
    mem.addr = 0;
    mem.len = 0;
    intr_no = 0;
    return;
  }

  /**
   * @brief resource_t 输出
   * @param  _os             输出流
   * @param  _res            要输出的 resource_t
   * @return std::ostream&   输出流
   */
  friend std::ostream& operator<<(std::ostream& _os, const resource_t& _res) {
    printf("%s: ", _res.name);
    if (_res.type & MEM) {
      printf("MEM(0x%p, 0x%p)", _res.mem.addr, _res.mem.len);
    }
    if (_res.type & INTR_NO) {
      printf(", INTR_NO(0x%p)", _res.intr_no);
    }
    return _os;
  }
};

class fdt_parser final {
 private:
  /// @see devicetree-specification-v0.3.pdf#5.4
  /// node 开始标记
  static constexpr const uint32_t FDT_BEGIN_NODE = 0x1;
  /// node 结束标记
  static constexpr const uint32_t FDT_END_NODE = 0x2;
  /// 属性开始标记
  static constexpr const uint32_t FDT_PROP = 0x3;
  /// 无效标记
  static constexpr const uint32_t FDT_NOP = 0x4;
  /// 数据区结束标记
  static constexpr const uint32_t FDT_END = 0x9;

  /// @see devicetree-specification-v0.3.pdf#5.1
  /// 魔数
  static constexpr const uint32_t FDT_MAGIC = 0xD00DFEED;
  /// 版本 17
  static constexpr const uint32_t FDT_VERSION = 0x11;

  /**
   * @brief fdt 格式头
   */
  struct fdt_header_t {
    /// 魔数，0xD00DFEED
    uint32_t magic;
    /// 整个 fdt 结构的大小
    uint32_t totalsize;
    /// 数据区相对 header 地址的偏移
    uint32_t off_dt_struct;
    /// 字符串区相对 header 地址的偏移
    uint32_t off_dt_strings;
    /// 内存保留相对 header 地址的偏移
    uint32_t off_mem_rsvmap;
    /// fdt 版本，17
    uint32_t version;
    /// 最新的兼容版本
    uint32_t last_comp_version;
    /// 启动 cpu 的 id
    uint32_t boot_cpuid_phys;
    /// 字符串区长度
    uint32_t size_dt_strings;
    /// 数据区长度
    uint32_t size_dt_struct;
  };

  /**
   * @brief 内存保留区信息
   * @see devicetree-specification-v0.3.pdf#5.3.2
   */
  struct fdt_reserve_entry_t {
    /// 地址
    uint32_t addr_be;
    uint32_t addr_le;
    /// 长度
    uint32_t size_be;
    uint32_t size_le;
  };

  /**
   * @brief 属性节点格式，没有使用，仅用于参考
   */
  struct fdt_property_t {
    /// 第一个字节表示 type
    uint32_t tag;
    /// 表示 property value 的长度，byte 为单位
    uint32_t len;
    /// property 的名称存放在 string block 区域，nameoff 表示其在 string
    /// block 的偏移
    uint32_t nameoff;
    /// property value值，作为额外数据以'\0'结尾的字符串形式存储 structure
    /// block, 32 - bits对齐，不够的位用 0x0 补齐
    uint32_t data[0];
  };

  /// 路径最大深度
  static constexpr const size_t MAX_DEPTH = 16;
  /// 最大节点数
  static constexpr const size_t MAX_NODES_COUNT = 128;
  /// 最大属性数
  static constexpr const size_t PROP_MAX_COUNT = 16;

  /**
   * @brief dtb 信息
   */
  struct dtb_info_t {
    /// dtb 头
    fdt_header_t* header;
    /// 保留区
    fdt_reserve_entry_t* reserved;
    /// 数据区
    uintptr_t data;
    /// 字符区
    uintptr_t str;
  };

  /**
   * @brief 属性信息
   */
  struct prop_t {
    /// 属性名
    char* name;
    /// 属性地址
    uintptr_t addr;
    /// 属性长度
    size_t len;
  };

  /**
   * @brief 路径
   */
  struct path_t {
    /// 当前路径
    char* path[MAX_DEPTH];
    /// 长度
    size_t len;

    /**
     * @brief 全部匹配
     * @param  _path           要比较的路径
     * @return true            相同
     * @return false           不同
     */
    bool operator==(const path_t* _path) {
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

    bool operator==(const char* _path) {
      // 路径必须以 ‘/’ 开始
      if (_path[0] != '/') {
        return false;
      }
      // 记录当前 _path 处理到的下标
      size_t tmp = 0;
      for (size_t i = 0; i < len; i++) {
        if (strncmp(path[i], &_path[tmp + i], strlen(path[i])) != 0) {
          return false;
        }
        tmp += strlen(path[i]);
      }
      return true;
    }
  };

  /**
   * @brief 节点数据
   */
  struct node_t {
    /// 节点路径
    path_t path;
    /// 节点地址
    uint32_t* addr;
    /// 父节点
    node_t* parent;
    /// 中断父节点
    node_t* interrupt_parent;
    /// 1 cell == 4 bytes
    /// 地址长度 单位为 bytes
    uint32_t address_cells;
    /// 长度长度 单位为 bytes
    uint32_t size_cells;
    /// 中断长度 单位为 bytes
    uint32_t interrupt_cells;
    uint32_t phandle;
    /// 路径深度
    uint8_t depth;
    /// 属性
    prop_t props[PROP_MAX_COUNT];
    /// 属性数
    size_t prop_count;
    /// 查找节点中的键值对
    bool find(const char* _prop_name, const char* _val);
  };

  /**
   * @brief 迭代变量
   */
  struct iter_data_t {
    /// 路径，不包括节点名
    path_t path;
    /// 节点地址
    uint32_t* addr;
    /// 节点类型
    uint32_t type;
    /// 如果节点类型为 PROP， 保存节点属性名
    char* prop_name;
    /// 如果节点类型为 PROP， 保存属性长度 单位为 byte
    uint32_t prop_len;
    /// 如果节点类型为 PROP， 保存属性地址
    uint32_t* prop_addr;
    /// 在 nodes 数组的下标
    uint8_t nodes_idx;
  };

  // 部分属性及格式
  /// @see devicetree-specification-v0.3#2.3
  /// @see devicetree-specification-v0.3#2.4.1
  /**
   * @brief 格式
   */
  enum dt_fmt_t {
    /// 未知
    FMT_UNKNOWN = 0,
    /// 空
    FMT_EMPTY,
    /// uint32_t
    FMT_U32,
    /// uint64_t
    FMT_U64,
    /// 字符串
    FMT_STRING,
    /// phandle
    FMT_PHANDLE,
    /// 字符串列表
    FMT_STRINGLIST,
    /// reg
    FMT_REG,
    /// ranges
    FMT_RANGES,
  };

  /**
   * @brief 用于 get_fmt()
   */
  struct dt_prop_fmt_t {
    /// 属性名
    char* prop_name;
    /// 格式
    enum dt_fmt_t fmt;
  };

  /**
   * @brief 用于 get_fmt()
   * @see 格式信息请查看 devicetree-specification-v0.3#2.3,#2.4 等部分
   */
  static constexpr const dt_prop_fmt_t props[] = {
      {.prop_name = (char*)"", .fmt = FMT_EMPTY},
      {.prop_name = (char*)"compatible", .fmt = FMT_STRINGLIST},
      {.prop_name = (char*)"model", .fmt = FMT_STRING},
      {.prop_name = (char*)"phandle", .fmt = FMT_U32},
      {.prop_name = (char*)"status", .fmt = FMT_STRING},
      {.prop_name = (char*)"#address-cells", .fmt = FMT_U32},
      {.prop_name = (char*)"#size-cells", .fmt = FMT_U32},
      {.prop_name = (char*)"#interrupt-cells", .fmt = FMT_U32},
      {.prop_name = (char*)"reg", .fmt = FMT_REG},
      {.prop_name = (char*)"virtual-reg", .fmt = FMT_U32},
      {.prop_name = (char*)"ranges", .fmt = FMT_RANGES},
      {.prop_name = (char*)"dma-ranges", .fmt = FMT_RANGES},
      {.prop_name = (char*)"name", .fmt = FMT_STRING},
      {.prop_name = (char*)"device_type", .fmt = FMT_STRING},
      {.prop_name = (char*)"interrupts", .fmt = FMT_U32},
      {.prop_name = (char*)"interrupt-parent", .fmt = FMT_PHANDLE},
      {.prop_name = (char*)"interrupt-controller", .fmt = FMT_EMPTY},
      {.prop_name = (char*)"value", .fmt = FMT_U32},
      {.prop_name = (char*)"offset", .fmt = FMT_U32},
      {.prop_name = (char*)"regmap", .fmt = FMT_U32},
  };

  /**
   * @brief 查找 _prop_name 在 dt_fmt_t 的索引
   * @param  _prop_name      要查找的属性
   * @return dt_fmt_t        在 dt_fmt_t 中的索引
   */
  dt_fmt_t get_fmt(const char* _prop_name) {
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

  /**
   * @brief phandles 与 node 的映射关系
   */
  struct phandle_map_t {
    uint32_t phandle;
    node_t* node;
  };

  /**
   * @brief 输出 reserved 内存
   */
  void dtb_mem_reserved(void) {
    fdt_reserve_entry_t* entry = dtb_info.reserved;
    if (entry->addr_le || entry->size_le) {
      // 目前没有考虑这种情况，先报错
      assert(0);
    }
    return;
  }

  /**
   * @brief 迭代函数
   * @param  _cb_flags       要迭代的标签
   * @param  _cb             迭代操作
   * @param  _data           要传递的数据
   * @param _addr            dtb 数据地址
   */
  void dtb_iter(uint8_t _cb_flags,
                std::function<bool(const iter_data_t*, void*)> _cb, void* _data,
                uintptr_t _addr) {
    // 迭代变量
    iter_data_t iter;
    // 路径深度
    iter.path.len = 0;
    // 数据地址
    iter.addr = (uint32_t*)_addr;
    // 节点索引
    iter.nodes_idx = 0;
    // 开始 flag
    bool begin = true;

    while (1) {
      //
      iter.type = be32toh(iter.addr[0]);
      switch (iter.type) {
        case FDT_NOP: {
          // 跳过 type
          iter.addr++;
          break;
        }
        case FDT_BEGIN_NODE: {
          // 第 len 深底的名称
          iter.path.path[iter.path.len] = (char*)(iter.addr + 1);
          // 深度+1
          iter.path.len++;
          iter.nodes_idx = begin ? 0 : (iter.nodes_idx + 1);
          begin = false;
          if (_cb_flags & DT_ITER_BEGIN_NODE) {
            if (_cb(&iter, _data)) {
              return;
            }
          }
          // 跳过 type
          iter.addr++;
          // 跳过 name
          iter.addr += ALIGN(strlen((char*)iter.addr) + 1, 4) / 4;
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
          iter.prop_len = be32toh(iter.addr[1]);
          iter.prop_name = (char*)(dtb_info.str + be32toh(iter.addr[2]));
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

  /**
   * @brief 查找 phandle 映射
   * @param  _phandle        要查找的 phandle
   * @return node_t*         _phandle 指向的节点
   */
  node_t* get_phandle(uint32_t _phandle) {
    // 在 phandle_map 中寻找对应的节点
    for (size_t i = 0; i < phandle_map_count; i++) {
      if (phandle_map[i].phandle == _phandle) {
        return phandle_map[i].node;
      }
    }
    return nullptr;
  }

  /**
   * @brief 初始化节点
   * @param  _iter           迭代变量
   * @param  _data           数据，未使用
   * @return true            成功
   * @return false           失败
   */
  bool dtb_init_cb(const iter_data_t* _iter, void* _data) {
    // 索引
    size_t idx = _iter->nodes_idx;
    // 根据类型
    switch (_iter->type) {
      // 开始
      case FDT_BEGIN_NODE: {
        // 设置节点基本信息
        nodes[idx].path = _iter->path;
        nodes[idx].addr = _iter->addr;
        nodes[idx].depth = _iter->path.len;
        // 设置默认值
        nodes[idx].address_cells = 2;
        nodes[idx].size_cells = 2;
        nodes[idx].interrupt_cells = 0;
        nodes[idx].phandle = 0;
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
          nodes[idx].address_cells = be32toh(_iter->addr[3]);
        } else if (strcmp(_iter->prop_name, "#size-cells") == 0) {
          nodes[idx].size_cells = be32toh(_iter->addr[3]);
        } else if (strcmp(_iter->prop_name, "#interrupt-cells") == 0) {
          nodes[idx].interrupt_cells = be32toh(_iter->addr[3]);
        }
        // phandle 信息
        else if (strcmp(_iter->prop_name, "phandle") == 0) {
          nodes[idx].phandle = be32toh(_iter->addr[3]);
          // 更新 phandle_map
          phandle_map[phandle_map_count].phandle = nodes[idx].phandle;
          phandle_map[phandle_map_count].node = &nodes[idx];
          phandle_map_count++;
        }
        // 添加属性
        nodes[idx].props[nodes[idx].prop_count].name = _iter->prop_name;
        nodes[idx].props[nodes[idx].prop_count].addr =
            (uintptr_t)(_iter->addr + 3);
        nodes[idx].props[nodes[idx].prop_count].len = be32toh(_iter->addr[1]);
        nodes[idx].prop_count++;
        break;
      }
      case FDT_END_NODE: {
        // 节点数+1
        nodes_count = idx + 1;
        break;
      }
    }
    // 返回 false 表示需要迭代全部节点
    return false;
  }

  /**
   * @brief 初始化中断信息
   * @param  _iter           迭代变量
   * @param  _data           数据，未使用
   * @return true            成功
   * @return false           失败
   */
  bool dtb_init_interrupt_cb(const iter_data_t* _iter, void* _data) {
    uint8_t idx = _iter->nodes_idx;
    uint32_t phandle;
    node_t* parent;
    // 设置中断父节点
    if (strcmp(_iter->prop_name, "interrupt-parent") == 0) {
      phandle = be32toh(_iter->addr[3]);
      parent = get_phandle(phandle);
      // 没有找到则报错
      assert(parent != nullptr);
      nodes[idx].interrupt_parent = parent;
    }
    // 返回 false 表示需要迭代全部节点
    return false;
  }

  /**
   * @brief 输出不定长度的数据
   * @param  _iter           要输出的迭代对象
   * @param  _cells          cell 数
   * @param  _len            长度
   * @todo 补充说明
   */
  void print_attr_propenc(const iter_data_t* _iter, size_t* _cells,
                          size_t _len) {
    // 字节总数
    uint32_t entry_size = 0;
    // 属性长度
    uint32_t remain = _iter->prop_len;
    // 属性数据
    uint32_t* reg = _iter->prop_addr;
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
          printf("0x%X ", be32toh(*reg));
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

  /**
   * @brief 填充 resource_t
   * @param  _resource       被填充的
   * @param  _node           源节点
   * @param  _prop           填充的数据
   */
  void fill_resource(resource_t* _resource, const node_t* _node,
                     const prop_t* _prop) {
    // 如果 _resource 名称为空则使用 compatible，如果没有找到则使用 _node 路径
    if (_resource->name == nullptr) {
      for (size_t i = 0; i < _node->prop_count; i++) {
        if (strcmp(_node->props[i].name, "compatible") == 0) {
          _resource->name = (char*)_node->props[i].addr;
        }
        if (_resource->name == nullptr) {
          _resource->name = _node->path.path[_node->path.len - 1];
        }
      }
    }
    // 内存类型
    if ((_resource->type & resource_t::MEM) && (_resource->mem.len == 0)) {
      // 根据 address_cells 与 size_cells 填充
      // resource 一般来说两者是相等的
      if (_node->parent->address_cells == 1) {
        assert(_node->parent->size_cells == 1);
        _resource->mem.addr = be32toh(((uint32_t*)_prop->addr)[0]);
        _resource->mem.len = be32toh(((uint32_t*)_prop->addr)[1]);
      } else if (_node->parent->address_cells == 2) {
        assert(_node->parent->size_cells == 2);
        _resource->mem.addr = be32toh(((uint32_t*)_prop->addr)[0]) +
                              be32toh(((uint32_t*)_prop->addr)[1]);
        _resource->mem.len = be32toh(((uint32_t*)_prop->addr)[2]) +
                             be32toh(((uint32_t*)_prop->addr)[3]);
      } else {
        assert(0);
      }
    } else if (_resource->type & resource_t::INTR_NO) {
      _resource->intr_no = be32toh(((uint32_t*)_prop->addr)[0]);
    }
    return;
  }

  /**
   * @brief 通过路径寻找节点
   * @param  _path            路径
   * @return node_t*          找到的节点
   */
  node_t* find_node_via_path(const char* _path) {
    node_t* res = nullptr;
    // 遍历 nodes
    for (size_t i = 0; i < nodes_count; i++) {
      // 如果 nodes[i] 中有属性/值对符合要求
      if (nodes[i].path == _path) {
        // 设置返回值
        res = &nodes[i];
      }
    }
    return res;
  }

 public:
  /// dtb 信息
  dtb_info_t dtb_info;
  /// 节点数组
  node_t nodes[MAX_NODES_COUNT];
  /// 有效 node 数量
  size_t nodes_count;
  /// phandle 数组
  phandle_map_t phandle_map[MAX_NODES_COUNT];
  /// 有效 phandle 数量
  size_t phandle_map_count;

  // 用于控制处理哪些属性
  /// 处理节点开始
  static constexpr const uint8_t DT_ITER_BEGIN_NODE = 0x01;
  /// 处理节点结束
  static constexpr const uint8_t DT_ITER_END_NODE = 0x02;
  /// 处理节点属性
  static constexpr const uint8_t DT_ITER_PROP = 0x04;

  /**
   * @brief 初始化
   * @return true            成功
   * @return false           失败
   */
  uintptr_t boot_info_addr;
  size_t boot_info_size;

  bool dtb_init(void) {
    // 头信息
    dtb_info.header = (fdt_header_t*)boot_info_addr;
    // 魔数
    assert(be32toh(dtb_info.header->magic) == FDT_MAGIC);
    // 版本
    assert(be32toh(dtb_info.header->version) == FDT_VERSION);
    // 设置大小
    boot_info_size = be32toh(dtb_info.header->totalsize);
    // 内存保留区
    dtb_info.reserved =
        (fdt_reserve_entry_t*)(boot_info_addr +
                               be32toh(dtb_info.header->off_mem_rsvmap));
    // 数据区
    dtb_info.data = boot_info_addr + be32toh(dtb_info.header->off_dt_struct);
    // 字符区
    dtb_info.str = boot_info_addr + be32toh(dtb_info.header->off_dt_strings);
    // 检查保留内存
    dtb_mem_reserved();
    // 初始化 map
    bzero(nodes, sizeof(nodes));
    bzero(phandle_map, sizeof(phandle_map));
    // // 初始化节点的基本信息
    // dtb_iter(DT_ITER_BEGIN_NODE | DT_ITER_END_NODE | DT_ITER_PROP,
    //          std::bind(&dtb_init_cb, this, std::placeholders::_1,
    //                    std::placeholders::_2),
    //          nullptr, boot_info_addr);
    // // 中断信息初始化，因为需要查找 phandle，所以在基本信息初始化完成后进行
    // dtb_iter(DT_ITER_PROP,
    //          std::bind(&dtb_init_interrupt_cb, this, std::placeholders::_1,
    //                    std::placeholders::_2),
    //          nullptr, boot_info_addr);
// #define DEBUG
#ifdef DEBUG
    // 输出所有信息
    for (size_t i = 0; i < nodes[0].count; i++) {
      std::cout << nodes[i].path << ": " << std::endl;
      for (size_t j = 0; j < nodes[i].prop_count; j++) {
        printf("%s: ", nodes[i].props[j].name);
        for (size_t k = 0; k < nodes[i].props[j].len / 4; k++) {
          printf("0x%X ", be32toh(((uint32_t*)nodes[i].props[j].addr)[k]));
        }
        printf("\n");
      }
    }
#undef DEBUG
#endif
    return true;
  }

  /**
   * @brief 根据路径查找节点，返回使用的资源
   * @param  _path            节点路径
   * @param  _resource        资源
   * @return true             成功
   * @return false            失败
   */
  bool find_via_path(const char* _path, resource_t* _resource) {
    // 找到节点
    auto node = find_node_via_path(_path);
    // std::cout << node->path << std::endl;
    // 找到 reg
    for (size_t i = 0; i < node->prop_count; i++) {
      // printf("node->props[i].name: %s\n", node->props[i].name);
      if (strcmp(node->props[i].name, "reg") == 0) {
        // 填充数据
        _resource->type |= resource_t::MEM;
        fill_resource(_resource, node, &node->props[i]);
      } else if (strcmp(node->props[i].name, "interrupts") == 0) {
        // 填充数据
        _resource->type |= resource_t::INTR_NO;
        fill_resource(_resource, node, &node->props[i]);
      }
    }
    return true;
  }

  /**
   * @brief 根据节点名进行前缀查找
   * @param  _prefix          要查找节点的前缀
   * @param  _resource        结果数组
   * @return size_t           _resource 长度
   * @note 根据节点 @ 前的名称查找，可能返回多个 resource
   */
  size_t find_via_prefix(const char* _prefix, resource_t* _resource) {
    size_t res = 0;
    // 遍历所有节点，查找
    // 由于 @ 均为最底层节点，所以直接比较最后一级即可
    for (size_t i = 0; i < nodes_count; i++) {
      if (strncmp(nodes[i].path.path[nodes[i].path.len - 1], _prefix,
                  strlen(_prefix)) == 0) {
        // 找到 reg
        for (size_t j = 0; j < nodes[i].prop_count; j++) {
          if (strcmp(nodes[i].props[j].name, "reg") == 0) {
            _resource[res].type |= resource_t::MEM;
            // 填充数据
            fill_resource(&_resource[res], &nodes[i], &nodes[i].props[j]);
          } else if (strcmp(nodes[i].props[j].name, "interrupts") == 0) {
            _resource[res].type |= resource_t::INTR_NO;
            // 填充数据
            fill_resource(&_resource[res], &nodes[i], &nodes[i].props[j]);
          }
        }
        res++;
      }
    }
    return res;
  }

  /**
   * @brief iter 输出
   * @param  _os             输出流
   * @param  _iter           要输出的 iter
   * @return std::ostream&   输出流
   */
  // friend std::ostream& operator<<(std::ostream& _os, const iter_data_t&
  // _iter) {
  //   // 输出路径
  //   _os << _iter.path << ": ";
  //   // 根据属性类型输出
  //   switch (fdt_parser::get_instance().get_fmt(_iter.prop_name)) {
  //     // 未知
  //     case fdt_parser::FMT_UNKNOWN: {
  //       printf("%s: (unknown format, len=0x%X)", _iter.prop_name,
  //              _iter.prop_len);
  //       break;
  //     }
  //     // 空
  //     case fdt_parser::FMT_EMPTY: {
  //       _os << _iter.prop_name << ": (empty)";
  //       break;
  //     }
  //     // 32 位整数
  //     case fdt_parser::FMT_U32: {
  //       printf("%s: 0x%X", _iter.prop_name,
  //              be32toh(*(uint32_t*)_iter.prop_addr));
  //       break;
  //     }
  //     // 64 位整数
  //     case fdt_parser::FMT_U64: {
  //       printf("%s: 0x%p", _iter.prop_name,
  //              be32toh(*(uint64_t*)_iter.prop_addr));
  //       break;
  //     }
  //     // 字符串
  //     case fdt_parser::FMT_STRING: {
  //       _os << _iter.prop_name << ": " << (char*)_iter.prop_addr;
  //       break;
  //     }
  //     // phandle
  //     case fdt_parser::FMT_PHANDLE: {
  //       uint32_t phandle = be32toh(_iter.addr[3]);
  //       fdt_parser::node_t* ref =
  //           fdt_parser::get_instance().get_phandle(phandle);
  //       if (ref != nullptr) {
  //         printf("%s: <phandle &%s>", _iter.prop_name, ref->path.path[0]);
  //       } else {
  //         printf("%s: <phandle 0x%X>", _iter.prop_name, phandle);
  //       }
  //       break;
  //     }
  //     // 字符串列表
  //     case fdt_parser::FMT_STRINGLIST: {
  //       size_t len = 0;
  //       char* str = (char*)_iter.prop_addr;
  //       _os << _iter.prop_name << ": [";
  //       while (len < _iter.prop_len) {
  //         // 用 "" 分隔
  //         _os << "\"" << str << "\"";
  //         len += strlen(str) + 1;
  //         str = (char*)((uint8_t*)_iter.prop_addr + len);
  //       }
  //       _os << "]";
  //       break;
  //     }
  //     // reg，不定长的 32 位数据
  //     case fdt_parser::FMT_REG: {
  //       // 获取节点索引
  //       uint8_t idx = _iter.nodes_idx;
  //       // 全部 cells 大小
  //       // devicetree-specification-v0.3.pdf#2.3.6
  //       size_t cells[] = {
  //           fdt_parser::nodes[idx].parent->address_cells,
  //           fdt_parser::nodes[idx].parent->size_cells,
  //       };
  //       // 调用辅助函数进行输出
  //       fdt_parser::get_instance().print_attr_propenc(
  //           &_iter, cells, sizeof(cells) / sizeof(size_t));
  //       break;
  //     }
  //     case fdt_parser::FMT_RANGES: {
  //       // 获取节点索引
  //       uint8_t idx = _iter.nodes_idx;
  //       // 全部 cells 大小
  //       // devicetree-specification-v0.3.pdf#2.3.8
  //       size_t cells[] = {
  //           fdt_parser::nodes[idx].address_cells,
  //           fdt_parser::nodes[idx].parent->address_cells,
  //           fdt_parser::nodes[idx].size_cells,
  //       };
  //       // 调用辅助函数进行输出
  //       fdt_parser::get_instance().print_attr_propenc(
  //           &_iter, cells, sizeof(cells) / sizeof(size_t));
  //       break;
  //     }
  //     default: {
  //       assert(0);
  //       break;
  //     }
  //   }
  //   return _os;
  // }

  /**
   * @brief 路径输出
   * @param  _os             输出流
   * @param  _path           要输出的 path
   * @return std::ostream&   输出流
   */
  friend std::ostream& operator<<(std::ostream& _os, const path_t& _path) {
    if (_path.len == 1) {
      _os << "/";
    }
    for (size_t i = 1; i < _path.len; i++) {
      _os << "/";
      _os << _path.path[i];
    }
    return _os;
  }
};

}  // namespace FDT_PARSER

#endif /* FDT_PARSER_SRC_INCLUDE_FDT_PARSER_H */
