#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
* client初始化函数
* @param: 配置文件路径
*/
extern int Init(const char* configpath);

/**
* 打开或创建文件
* @param: fs_id 文件系统id
* @param: ino 文件系统文件ino
* @param: flag 操作类型 (read:1)(write:2)
* @return: 返回BlobID对象
*/
extern void* OpenBlobFS(const char* fs_id,
                        const uint64_t ino,
                        int flag);

/**
* 打开或创建文件
* @param: bucket 对象桶名
* @param: path object对象路径
* @param: flag 操作类型 (read:1)(write:2)
* @return: 返回BlobID对象
*/
extern void* OpenBlobObject(const char* bucket,
                            const char* path,
                            int flag);

/**
* 同步模式读
* @param: BlobID为当前open返回的文件描述符
* @param: buf为当前待读取的缓冲区
* @param：offset文件内的偏移
* @parma：length为待读取的长度
* @parma：cb 回调函数指针
* @return: 成功返回读取字节数,否则返回小于0的错误码
*/
extern int ReadBlob(void* blob_id, char* buf,
                    off_t offset, size_t length,
                    void* cb);

/**
* 同步模式写
* @param: BlobID为当前open返回的文件描述符
* @param: buf为当前待写入的缓冲区
* @parma：length为待写入的长度
* @parma：sync直写模式
* @parma：cb 回调函数指针
* @return: 成功返回写入字节数,否则返回小于0的错误码
*/
extern int WriteBlob(void* blob_id, const char* buf,
                     size_t length, bool sync,
                     void* cb);

/**
* sync通过blobID找到对应的文件缓存下刷
* @param: blobID为当前open返回的文件描述符
* @return: 成功返回int::OK,否则返回小于0的错误码
*/
extern int SyncBlob(void* blob_id);

/**
* close通过blobID找到对应的文件进行关闭
* @param: blobID为当前open返回的文件描述符
* @return: 成功返回int::OK,否则返回小于0的错误码
*/
extern int CloseBlob(void* blob_id);

/**
* delete通过blobID找到对应的文件进行删除
* @param: blobID为当前open返回的文件描述符
* @return: 成功返回int::OK,否则返回小于0的错误码
*/
extern int DeleteBlob(void* blob_id);

/**
* 析构，回收资源
*/
extern void UnInit();

#ifdef __cplusplus
}
#endif