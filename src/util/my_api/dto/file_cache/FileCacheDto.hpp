#pragma once

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp-swagger/Types.hpp"

namespace my_api::dto {

#include OATPP_CODEGEN_BEGIN(DTO)

class CacheFileRequestDto : public oatpp::DTO {
    DTO_INIT(CacheFileRequestDto, DTO)

    DTO_FIELD(String, filename);
};

class CacheUploadRequestDto : public oatpp::DTO {
    DTO_INIT(CacheUploadRequestDto, DTO)

    DTO_FIELD(String, filename);
    DTO_FIELD(String, data);
};

class CacheUploadLocalRequestDto : public oatpp::DTO {
    DTO_INIT(CacheUploadLocalRequestDto, DTO)

    DTO_FIELD(String, file_name);
    DTO_FIELD(String, data);
};

class CacheUploadLocalMultipartDto : public oatpp::DTO {
    DTO_INIT(CacheUploadLocalMultipartDto, DTO)

    DTO_FIELD(String, file_name);
    DTO_FIELD(oatpp::swagger::Binary, file);

    DTO_FIELD_INFO(file_name) {
        info->description = "缓存中的目标文件名，可选；为空时使用上传文件原始文件名";
        info->required = false;
    }

    DTO_FIELD_INFO(file) {
        info->description = "要上传的二进制文件";
        info->required = true;
    }
};

class CacheListRequestDto : public oatpp::DTO {
    DTO_INIT(CacheListRequestDto, DTO)

    DTO_FIELD(String, folder_path);
};

class CacheCreateFolderRequestDto : public oatpp::DTO {
    DTO_INIT(CacheCreateFolderRequestDto, DTO)

    DTO_FIELD(String, folder_path);
    DTO_FIELD(String, new_folder_name);
};

#include OATPP_CODEGEN_END(DTO)

}  // namespace my_api::dto