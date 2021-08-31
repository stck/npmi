#ifndef NPM_TAR_HEADER_H
#define NPM_TAR_HEADER_H

namespace tar {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
  struct Header {
    char fileName[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char fileSize[12];
    char lastModification[12];
    char checksum[8];
    char typeFlag;
    char linkedFileName[100];

    char ustarIndicator[6];
    char ustarVersion[2];
    char ownerUserName[32];
    char ownerGroupName[32];
    char deviceMajorNumber[8];
    char deviceMinorNumber[8];
    char filenamePrefix[155];
    char padding[12];
    char firstContent;
  };
#pragma clang diagnostic pop
}  // namespace tar

#endif  //NPM_TAR_HEADER_H
