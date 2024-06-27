#pragma once
#include "Buffer.h"
#include "DatabaseHelper.h"
#include <mysql/mysql.h>

// 数据库操作类
class CMysqlClient
    : public CDatabaseClient
{

public:
    CMysqlClient()
    {
        bzero(&m_db, sizeof(m_db));
        m_bInit = false;
    }
    virtual ~CMysqlClient()
    {
        Close(); // 关闭数据库连接
    }

public:
    CMysqlClient(const CMysqlClient &) = delete;
    CMysqlClient &operator=(const CMysqlClient &) = delete;

public:
    // 连接数据库
    virtual int Connect(const KeyValue &args);
    // 执行sql语句
    virtual int Exec(const Buffer &sql);
    // 带结果的执行sql语句
    virtual int Exec(const Buffer &sql, Result __OUT__ &result, const _Table_ __IN__ &table);
    // 开启事务
    virtual int StartTransaction();
    // 提交事务
    virtual int CommitTransaction();
    // 回滚事务
    virtual int RollbackTransaction();
    // 关闭连接
    virtual int Close();
    // 是否连接 true表示连接中 false表示未连接
    virtual bool IsConnected();

private:
    MYSQL *m_db;
    bool m_bInit; // 默认是false 表示没有初始化 初始化后为true 表示已连接

private:
    // 内部类 封装（结果表，操作表）
    class ExecParam
    {
    public:
        ExecParam(CMysqlClient *obj, Result &result, const _Table_ &table)
            : obj(obj), result(result), table(table)
        {
        }
        CMysqlClient *obj;
        Result &result;
        const _Table_ &table;
    };
};

// 表操作类
class _mysql_table_ : public _Table_
{
public:
    _mysql_table_() : _Table_() {}
    _mysql_table_(const _mysql_table_ &table); // 拷贝构造
    virtual ~_mysql_table_();

    // 返回创建表的SQL语句
    virtual Buffer Create();
    // 删除表
    virtual Buffer Drop();

    // 表数据增删改查
    virtual Buffer Insert(const _Table_ &values);
    virtual Buffer Delete(const _Table_ &values);
    virtual Buffer Modify(const _Table_ &values);
    virtual Buffer Query(const Buffer &Condition = "");
    // 创建一个基于表的对象
    virtual PTable Copy() const;
    virtual void ClearFieldUsed();

public:
    // 获取表的全名
    virtual operator const Buffer() const;
};

// 字段操作类
class _mysql_field_ : public _Field_
{
public:
    _mysql_field_();

    _mysql_field_( // 有参构造
        int ntype,
        const Buffer &name,
        unsigned attr,
        const Buffer &type,
        const Buffer &size,
        const Buffer &default_,
        const Buffer &check);
    _mysql_field_(const _mysql_field_ &field); // 拷贝构造
    virtual ~_mysql_field_();

    // 返回创建字段语句 例如 "id INT PRIMARY_KEY NOT NULL AUTOINCREMENT "
    virtual Buffer Create();

    // 根据 字段类型nType 设置Value值
    virtual void LoadFromStr(const Buffer &str);

    // where条件语句使用
    virtual Buffer toEqualExp() const; // 返回 字段名=值
    virtual Buffer toSqlStr() const;   // 返回 值

    // 重载类型转换 返回字段的全名
    virtual operator const Buffer() const;

private:
    // 二进制转16进制
    Buffer Str2Hex(const Buffer &data) const;
};

// 声明表预定义
// name为表类型  base为父类型
#define DECLARE_TABLE_CLASS(name, base)                                 \
    class name : public base                                            \
    {                                                                   \
    public:                                                             \
        virtual PTable Copy() const { return PTable(new name(*this)); } \
        name() : base()                                                 \
        {                                                               \
            Name = #name;

#define DECLARE_MYSQL_FIELD(ntype, name, attr, type, size, default_, check)               \
    {                                                                                     \
        PField field(new _mysql_field_(ntype, #name, attr, type, size, default_, check)); \
        FieldDefine.push_back(field);                                                     \
        Fields[#name] = field;                                                            \
    }

#define DECLARE_TABLE_CLASS_EDN() \
    }                             \
    }                             \
    ;