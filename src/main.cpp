#include "EdoyunPlayerServer.h"
// 日志服务器进程
int CreateLogServer(CProcess *proc)
{

    // printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    CLoggerServer server;
    int ret = server.Start(); // 开启服务器
    if (ret != 0)
    {
        printf("%s(%d):<%s> pid=%d errno:%d msg:%s\n",
               __FILE__, __LINE__, __FUNCTION__, getpid(), errno, strerror(errno));
    }
    int fd = 0;
    while (true)
    {
        ret = proc->RecvFD(fd);
        printf("%s(%d):<%s> fd=%d\n", __FILE__, __LINE__, __FUNCTION__, fd);
        if (fd <= 0)
            break;
    }
    ret = server.Close();
    // printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    return 0;
}

// 客户端服务器
int CreateCLientServer(CProcess *proc)
{
    // printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    int fd = -1;
    int ret = proc->RecvFD(fd);
    if (ret == -1)
    {
        return ret;
    }
    // printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    // printf("%s(%d):<%s> fd=%d\n", __FILE__, __LINE__, __FUNCTION__, fd);
    sleep(1);
    char buf[20];
    lseek(fd, 0, SEEK_SET);
    read(fd, buf, sizeof(buf));
    // printf("%s(%d):<%s> buf=%s\n", __FILE__, __LINE__, __FUNCTION__, buf);
    return 0;
}

int LogTest()
{
    char buffer[] = "hello edoyun 肖老师";
    usleep(1000 * 100);
    TRACEI("here is log %d %c %f %g %s 哈哈 嘻嘻 易道云", 10, 'A', 1.0f, 2.0, buffer);
    DUMPD((void *)buffer, (size_t)sizeof(buffer));
    LOGE << 100 << " " << 'A' << " " << 0.12345f << " " << 1.2345 << " " << buffer << " "
         << "易道云编程";
    return 0;
}
int oldtest()
{
    //  CProcess::SwitchDeamon(); // 转到守护进程
    CProcess proclog, procclients;
    printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    proclog.SetEntryFunction(CreateLogServer, &proclog); // 设置日志服务器进程入口函数
    int ret = proclog.CreateSubProcess();                // 创建子进程
    if (ret != 0)
    {
        printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
        return -1;
    }

    // LogTest(); // 发送日志
    CThread thread(LogTest);
    thread.Start();

    printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    procclients.SetEntryFunction(CreateCLientServer, &procclients); // 设置客户端服务器入口函数
    ret = procclients.CreateSubProcess();                           // 创建子进程
    if (ret != 0)
    {
        printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
        return -2;
    }
    printf("%s(%d):<%s> pid=%d\n", __FILE__, __LINE__, __FUNCTION__, getpid());
    // usleep(100000);
    //  int fd = open("./text.txt", O_RDWR | O_CREAT | O_APPEND, 0644);
    //  printf("%s(%d):<%s> fd=%d\n", __FILE__, __LINE__, __FUNCTION__, fd);
    //  if (fd == -1)
    //      return -3;
    //  ret = procclients.SendFD(fd); // 发送套接字

    // printf("%s(%d):<%s> ret=%d\n", __FILE__, __LINE__, __FUNCTION__, ret);
    // write(fd, "xiaojie", 8);
    // close(fd);

    proclog.SendFD(-1); // 发送进程结束标志，结束进程
    (void)getchar();
    return 0;
}

int Main()
{
    // 日志
    int ret;
    CProcess proclog;                                          // 日志对象
    ret = proclog.SetEntryFunction(CreateLogServer, &proclog); // 设置日志服务器子进程入口函数
    ERR_RETURN(ret, -1);
    ret = proclog.CreateSubProcess(); // 创建日志服务器子进程
    ERR_RETURN(ret, -2);

    // 服务器
    CEdoyunPlayerServer business(2); // 业务对象
    CServer server;                  // 服务器对象
    ret = server.Init(&business);    // 初始化服务器
    ERR_RETURN(ret, -3);
    ret = server.Run(); // 运行服务器
    ERR_RETURN(ret, -4);
    return 0;
}

#include "HttpParser.h"
int http_test()
{
    Buffer str = "GET /favicon.ico HTTP/1.1\r\n"
                 "Host: 0.0.0.0=5000\r\n"
                 "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
                 "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*; q = 0.8\r\n"
                 "Accept-Language: en-us,en;q=0.5\r\n"
                 "Accept-Encoding: gzip,deflate\r\n"
                 "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
                 "Keep-Alive: 300\r\n"
                 "Connection: keep-alive\r\n"
                 "\r\n";
    CHttpParser parser;
    size_t size = parser.Parser(str);
    if (parser.Errno() != 0)
    {
        printf("errno %d\n", parser.Errno());
        return 1;
    }
    if (size != 368)
    {
        printf("size :%ld\n", size);
        return -2;
    }
    printf("method %d url %s\n", parser.Method(), (char *)parser.Url());

    str = "GET /favicon.ico HTTP/1.1\r\n"
          "Host: 0.0.0.0=5000\r\n"
          "User-Agent,deflate\r\n"
          "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
          "Keep-Alive: 300\r\n"
          "Connection: keep-alive\r\n"
          "\r\n";

    size = parser.Parser(str);
    printf("errno %d size %ld\n", parser.Errno(), size);
    if (parser.Errno() != 0x7F)
    {
        return -3;
    }
    if (size != 0)
        return -4;

    UrlParser url1("https://www.baidu.com/s?ie=utf8&oe=utf8&wd=httplib&tn=98010089_dg&ch=3");
    int ret = url1.Parser();
    if (ret != 0)
    {
        printf("urlparser1 faild:%d\n", ret);
        return -5;
    }

    printf("ie=%s\n except:utf8", (char *)url1["ie"]);
    printf("oe=%s except:utf8\n", (char *)url1["oe"]);
    printf("wd=%s except:httplib\n", (char *)url1["wd"]);
    printf("tn=%s except:98010089_dg\n", (char *)url1["tn"]);
    printf("ch=%s except:3\n", (char *)url1["ch"]);

    UrlParser url2("http://127.0.0.1:19811/?time=144000&salt=9527&user=test&sign=1234567890abcdef");
    ret = url2.Parser();
    if (ret != 0)
    {
        printf("urlparser2 faild:%d\n", ret);
        return -6;
    }
    printf("time=%s except:144000\n", (char *)url1["time"]);
    printf("salt=%s except:9527\n", (char *)url1["salt"]);
    printf("user=%s except:test\n", (char *)url1["user"]);
    printf("sign=%s except:1234567890abcdef\n", (char *)url1["sign"]);
    printf("host:%s port:%d\n", (char *)url2.Host(), url2.Port());
    return 0;
}

#include "Sqlite3Client.h"
// class user_test : public _sqlite3_table_//表格类
// {
// public:
//     virtual PTable Copy() const
//     {
//         return PTable(new user_test(*this));//拷贝表格
//     }

//     user_test() : _sqlite3_table_()//构造函数
//     {
//         Name = "suer_test";
//         {//字段1
//             PField field(new _sqlite3_field_(TYPE_INT, "user_id,", NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INT", "", "", ""));
//             FieldDefine.push_back(field);
//             Fields["user_id"] = field;
//         }

//         {//字段2
//             PField field(new _sqlite3_field_(TYPE_VARCHAR, "user_qq,", NOT_NULL, "VARCHAR", "(15)", "", ""));
//             FieldDefine.push_back(field);
//             Fields["user_qq"] = field;
//         }
//     }
// };

#include "Sqlite3Client.h"
DECLARE_TABLE_CLASS(user_test, _sqlite3_table_)
DECLARE_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")
DECLARE_FIELD(TYPE_VARCHAR, user_phone, NOT_NULL | DEFAULT, "VARCHAR", "(12)", "18888888888", "")
DECLARE_FIELD(TYPE_TEXT, user_name, 0, "TEXT", "", "", "")
DECLARE_TABLE_CLASS_EDN()
// DECLARE_FIELD(字段类型, 字段名, 约束条件, 数据库中数据类型, 字段大小, 默认值, check条件)

int sql_test()
{
    user_test test, value;
    printf("create:%s\n", (char *)test.Create());
    printf("Delete:%s\n", (char *)test.Delete(test));

    value.Fields["user_qq"]->LoadFromStr("1817619619");
    value.Fields["user_qq"]->Condition = SQL_INSERT;
    printf("Insert:%s\n", (char *)test.Insert(value));

    value.Fields["user_qq"]->LoadFromStr("123456789");
    value.Fields["user_qq"]->Condition = SQL_MODIFY;
    printf("Modify:%s\n", (char *)test.Modify(value));

    printf("Query:%s\n", (char *)test.Query());
    printf("Drop:%s\n", (char *)test.Drop());
    getchar();

    CDatabaseClient *pClient = new CSqlite3Client(); // 数据库操作对象
    KeyValue args;
    args["host"] = "test.db";     // 数据库创建参数
    pClient->Connect(args);       // 连接数据库
    pClient->Exec(test.Create()); // 创建表

    pClient->Exec(test.Delete(value)); // 删除数据

    value.Fields["user_qq"]->LoadFromStr("1817619619"); // 插入数据
    value.Fields["user_qq"]->Condition = SQL_INSERT;
    pClient->Exec(test.Insert(value));

    value.Fields["user_qq"]->LoadFromStr("123456789"); // 修改数据
    value.Fields["user_qq"]->Condition = SQL_MODIFY;
    pClient->Exec(test.Modify(value));

    // 有结果查询
    Result result;
    pClient->Exec(test.Query(), result, test);

    pClient->Exec(test.Drop()); // 删除表
    pClient->Close();           // 关闭数据库
    // getchar();
    return 0;
}

#include "MysqlClient.h"
#include "Sqlite3Client.h"
DECLARE_TABLE_CLASS(user_test_mysql, _mysql_table_)
DECLARE_MYSQL_FIELD(TYPE_INT, user_id, NOT_NULL | PRIMARY_KEY | AUTOINCREMENT, "INTEGER", "", "", "")
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_qq, NOT_NULL, "VARCHAR", "(15)", "", "")
DECLARE_MYSQL_FIELD(TYPE_VARCHAR, user_phone, NOT_NULL | DEFAULT, "VARCHAR", "(12)", "18888888888", "")
DECLARE_MYSQL_FIELD(TYPE_TEXT, user_name, 0, "TEXT", "", "", "")
DECLARE_TABLE_CLASS_EDN()
int mysql_test()
{

    user_test_mysql test, value;
    printf("create:%s\n", (char *)test.Create());
    printf("Delete:%s\n", (char *)test.Delete(test));

    value.Fields["user_qq"]->LoadFromStr("1817619619");
    value.Fields["user_qq"]->Condition = SQL_INSERT;
    printf("Insert:%s\n", (char *)test.Insert(value));

    value.Fields["user_qq"]->LoadFromStr("123456789");
    value.Fields["user_qq"]->Condition = SQL_MODIFY;
    printf("Modify:%s\n", (char *)test.Modify(value));

    printf("Query:%s\n", (char *)test.Query());
    printf("Drop:%s\n", (char *)test.Drop());
    getchar();

    CDatabaseClient *pClient = new CMysqlClient(); // 数据库操作对象
    KeyValue args;
    args["host"] = "192.168.153.1"; // 数据库创建参数
    args["user"] = "xbj";
    args["password"] = "xbj";
    args["port"] = 3306;
    args["db"] = "edoyun";

    pClient->Connect(args);                 // 连接数据库
    int ret = pClient->Exec(test.Create()); // 创建表
    if (ret != 0)
        printf("[%s]<%s>:%d ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret);

    ret = pClient->Exec(test.Delete(value)); // 删除数据
    if (ret != 0)
        printf("[%s]<%s>:%d ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret);

    value.Fields["user_qq"]->LoadFromStr("1817619619"); // 插入数据
    value.Fields["user_qq"]->Condition = SQL_INSERT;
    ret = pClient->Exec(test.Insert(value));
    if (ret != 0)
        printf("[%s]<%s>:%d ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret);

    value.Fields["user_qq"]->LoadFromStr("123456789"); // 修改数据
    value.Fields["user_qq"]->Condition = SQL_MODIFY;
    ret = pClient->Exec(test.Modify(value));
    if (ret != 0)
        printf("[%s]<%s>:%d ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret);

    // 有结果查询
    Result result;
    ret = pClient->Exec(test.Query(), result, test);
    if (ret != 0)
        printf("[%s]<%s>:%d ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret);

    ret = pClient->Exec(test.Drop()); // 删除表
    if (ret != 0)
        printf("[%s]<%s>:%d ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret);

    ret = pClient->Close(); // 关闭数据库
    if (ret != 0)
        printf("[%s]<%s>:%d ret=%d\n", __FILE__, __FUNCTION__, __LINE__, ret);
    return 0;
}

#include "encrypt/Crypto.h"
int crypto_test()
{
    Buffer data = "abcdef";
    data = Crypto::MD5(data);
    printf("%s\n", (char *)data);
    return 0;
}
int main()
{
    int ret = 0;
    // ret = http_test();
    // ret = sql_test();
    // ret = mysql_test();
    // ret = crypto_test();
    Main();
    printf("main:ret=%d\n", ret);
    return ret;
}