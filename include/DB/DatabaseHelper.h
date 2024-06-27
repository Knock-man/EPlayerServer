/*
数据库抽象类
表抽象类
字段抽象类
*/
#pragma once
#include "Buffer.h"
#include <map>
#include <list>
#include <memory>
#include <vector>
#include "Logger.h"

#define __OUT__ // 传出参数
#define __IN__  // 传入参数
#define __INOUT // 传入传出参数

class _Table_;
class _Field_;

using PTable = std::shared_ptr<_Table_>; // 表智能指针

using PField = std::shared_ptr<_Field_>; // 指向字段(列)的智能指针

using KeyValue = std::map<Buffer, Buffer>; // 用来初始化数据库连接参数 例如：args["数据库名"]="testdb"

using Result = std::list<PTable>;

// 数据库操作类
class CDatabaseClient
{

public:
    CDatabaseClient() {}
    virtual ~CDatabaseClient() {}

public:
    CDatabaseClient(const CDatabaseClient &) = delete;
    CDatabaseClient &operator=(const CDatabaseClient &) = delete;

public:
    // 连接数据库 通过映射表传递连接参数   例如：args["数据库名"] = 'testdb'
    virtual int Connect(const KeyValue &args) = 0;
    // 执行sql语句，无SELECT结果
    virtual int Exec(const Buffer &sql) = 0;
    // 执行sql语句,有SELECT结果             table为执行查询操作的表，通过 result返回查询结果
    virtual int Exec(const Buffer &sql, Result __OUT__ &result, const _Table_ __IN__ &table) = 0;
    // 开启事务
    virtual int StartTransaction() = 0;
    // 回滚事务
    virtual int RollbackTransaction() = 0;
    // 提交事务
    virtual int CommitTransaction() = 0;

    // 关闭数据库连接
    virtual int Close() = 0;
    // 数据库是否连接
    virtual bool IsConnected() = 0;
};

using FieldArray = std::vector<PField>;    // 存放字段
using FieldMap = std::map<Buffer, PField>; // 字段名=字段

// 表操作基类  返回sql语句
class _Table_
{
public:
    _Table_() {}
    virtual ~_Table_() {}

    // 创建表
    virtual Buffer Create() = 0;
    // 删除表
    virtual Buffer Drop() = 0;
    // 增删改查
    virtual Buffer Insert(const _Table_ &values) = 0;
    virtual Buffer Delete(const _Table_ &values) = 0;
    virtual Buffer Modify(const _Table_ &values) = 0;
    virtual Buffer Query(const Buffer &Condition = "") = 0;

    // 拷贝一个基于表的对象
    virtual PTable Copy() const = 0;
    virtual void ClearFieldUsed() = 0;

public:
    // 获取表的全名(类型转换运算符重载)  const Buffer tbName = _Table_; 或者 Buffer tbName = _Table_;
    virtual operator const Buffer() const = 0;

public:
    Buffer Database;        // 数据库名
    Buffer Name;            // 表名
    FieldArray FieldDefine; // 表的所有字段
    FieldMap Fields;        // 表的所有字段名和字段的映射
};

enum Condition
{
    SQL_INSERT = 1,   // 用来插入的字段
    SQL_MODIFY = 2,   // 用来修改的字段
    SQL_CONDITION = 4 // 用来当查询条件的字段
};

// 约束条件
enum Attr
{
    NONE = 0,
    NOT_NULL = 1,
    DEFAULT = 2,
    UNIQUE = 4,
    PRIMARY_KEY = 8,
    CHECK = 16,
    AUTOINCREMENT = 32
};

// 数据库类型
using SqlType = enum {
    TYPE_NULL = 0,
    TYPE_BOOL = 1,
    TYPE_INT = 2,
    TYPE_DATETIME = 4, // 日期和时间类型
    TYPE_REAL = 8,     // 实数
    TYPE_VARCHAR = 16, // 字符串
    TYPE_TEXT = 32,    // 文本
    TYPE_BLOB = 64     // 二进制数据 图像，音频，视频
};

// 字段基类
class _Field_
{
public:
    _Field_() {}
    _Field_(const _Field_ &field) // 拷贝构造
    {
        Name = field.Name;
        Type = field.Type;
        Attr = field.Attr;
        Default = field.Default;
        Check = field.Check;
    }
    virtual _Field_ &operator=(const _Field_ &field) // 赋值构造
    {
        if (this != &field)
        {
            Name = field.Name;
            Type = field.Type;
            Attr = field.Attr;
            Default = field.Default;
            Check = field.Check;
        }
        return *this;
    }
    virtual ~_Field_() {}

public:
    // 创建字段
    virtual Buffer Create() = 0;
    // 设置字段值
    virtual void LoadFromStr(const Buffer &str) = 0;
    // 返回 字段名=值
    virtual Buffer toEqualExp() const = 0;
    // 返回 值
    virtual Buffer toSqlStr() const = 0;
    // 类型转换运算符重载 取字段名 const Buffer FieldName = _Field_;
    virtual operator const Buffer() const = 0;

public:
    Buffer Name;    // 字段名
    Buffer Type;    // 字段数据类型
    Buffer Size;    // 字段大小
    unsigned Attr;  // 约束条件 见 Attr 枚举类型
    Buffer Default; // 默认值
    Buffer Check;   // 特殊约束 限制字段取值范围

public:
    // 字段用来做什么操作
    unsigned Condition; // 见 Condition 枚举类型
    // 字段值
    union
    {
        bool Bool;
        int Integer;
        double Double;
        Buffer *String;
    } Value;
    // 字段类型 见数据库类型枚举
    int nType;
    /*
        // 数据库字段类型
        using SqlType = enum {
            TYPE_NULL = 0,
            TYPE_BOOL = 1,
            TYPE_INT = 2,
            TYPE_DATETIME = 4,
            TYPE_REAL = 8,
            TYPE_VARCHAR = 16,
            TYPE_TEXT = 32,
            TYPE_BLOB = 64
        };
    */
};
