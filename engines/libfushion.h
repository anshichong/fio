#ifndef FUSHIONBS_SDK_LIBFUSHION_H_
#define FUSHIONBS_SDK_LIBFUSHION_H_

#include <string>

namespace fushionbs {
namespace common {
class BlobID;
}
}

typedef fushionbs::common::BlobID BlobID;

namespace fushionbs {
namespace client {

class BlobClient;

enum ClientOpenFlag {
    Read = 1,
    Write = 2
};

class FushionBSClient {
public:
    FushionBSClient();
    virtual ~FushionBSClient();

    /**
     * client初始化函数
     * @param: 配置文件路径
     */
    virtual int Init(const std::string& configpath);

    /**
     * 打开或创建文件
     * @param: fs_id 文件系统id
     * @param: ino 文件系统文件ino
     * @return: 返回BlobID对象
     */
    virtual const BlobID* OpenBlob(const std::string& fs_id,
                                   const uint64_t ino,
                                   ClientOpenFlag openflags);

    /**
     * 打开或创建文件
     * @param: bucket 对象桶名
     * @param: path object对象路径
     * @return: 返回BlobID对象
     */
    virtual const BlobID* OpenBlob(const std::string& bucket,
                                   const std::string& path,
                                   ClientOpenFlag openflags);

    /**
     * 同步模式读
     * @param: BlobID为当前open返回的文件描述符
     * @param: buf为当前待读取的缓冲区
     * @param：offset文件内的偏移
     * @parma：length为待读取的长度
     * @return: 成功返回读取字节数,否则返回小于0的错误码
     */
    virtual int ReadBlob(const BlobID* blob_id, char* buf,
                         off_t offset, size_t length);

    /**
     * 同步模式写
     * @param: BlobID为当前open返回的文件描述符
     * @param: buf为当前待写入的缓冲区
     * @parma：length为待写入的长度
     * @parma：sync直写模式
     * @return: 成功返回写入字节数,否则返回小于0的错误码
     */
    virtual int WriteBlob(const BlobID* blob_id, const char* buf,
                          size_t length, bool sync);

   /**
     * sync通过blobID找到对应的文件缓存下刷
     * @param: blobID为当前open返回的文件描述符
     * @return: 成功返回int::OK,否则返回小于0的错误码
     */
    virtual int SyncBlob(const BlobID* blob_id);

    /**
     * close通过blobID找到对应的文件进行关闭
     * @param: blobID为当前open返回的文件描述符
     * @return: 成功返回int::OK,否则返回小于0的错误码
     */
    virtual int CloseBlob(const BlobID* blob_id);

    /**
     * delete通过blobID找到对应的文件进行删除
     * @param: blobID为当前open返回的文件描述符
     * @return: 成功返回int::OK,否则返回小于0的错误码
     */
    virtual int DeleteBlob(const BlobID* blob_id);

    /**
     * 析构，回收资源
     */
    virtual void UnInit();

private:
    BlobClient* blob_client_;

    // 是否初始化成功
    bool inited_;
};

}  // namespace client
}  // namespace fushionbs

#endif  // FUSHIONBS_SDK_LIBFUSHION_H_
