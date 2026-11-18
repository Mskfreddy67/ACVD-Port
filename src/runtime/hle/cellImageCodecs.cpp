// HLE: cellPngDec, cellPngEnc, cellJpgEnc - Image Codecs (Stubs)
#include "runtime/hle.h"
#include "runtime/log.h"

namespace rt {
namespace {

// ---- cellPngDec ----
s32 cellPngDecCreate(Ptr<be32> handle, u32 /*param*/) {
  if (handle) *handle = 1;
  return CELL_OK;
}
s32 cellPngDecDestroy(u32 /*handle*/) { return CELL_OK; }
s32 cellPngDecOpen(u32 /*handle*/, u32 /*subHandle*/, u32 /*src*/, u32 /*info*/) { return CELL_OK; }
s32 cellPngDecClose(u32 /*handle*/, u32 /*subHandle*/) { return CELL_OK; }
s32 cellPngDecReadHeader(u32 /*handle*/, u32 /*subHandle*/, u32 /*info*/) { return CELL_OK; }
s32 cellPngDecSetParameter(u32 /*handle*/, u32 /*subHandle*/, u32 /*param*/, u32 /*info*/) { return CELL_OK; }
s32 cellPngDecDecodeData(u32 /*handle*/, u32 /*subHandle*/, u32 /*data*/, u32 /*inParam*/, u32 /*outData*/) { return CELL_OK; }

// ---- cellPngEnc ----
s32 cellPngEncOpen(Ptr<be32> handle, u32 /*param*/) {
  if (handle) *handle = 1;
  return CELL_OK;
}
s32 cellPngEncClose(u32 /*handle*/) { return CELL_OK; }
s32 cellPngEncQueryAttr(u32 /*handle*/, u32 /*attr*/) { return CELL_OK; }
s32 cellPngEncGetStreamInfo(u32 /*handle*/, u32 /*info*/) { return CELL_OK; }
s32 cellPngEncEncodePicture(u32 /*handle*/, u32 /*pic*/) { return CELL_OK; }
s32 cellPngEncWaitForInput(u32 /*handle*/) { return CELL_OK; }
s32 cellPngEncWaitForOutput(u32 /*handle*/) { return CELL_OK; }

// ---- cellJpgEnc ----
s32 cellJpgEncOpen(Ptr<be32> handle, u32 /*param*/) {
  if (handle) *handle = 1;
  return CELL_OK;
}
s32 cellJpgEncClose(u32 /*handle*/) { return CELL_OK; }
s32 cellJpgEncQueryAttr(u32 /*handle*/, u32 /*attr*/) { return CELL_OK; }
s32 cellJpgEncGetStreamInfo(u32 /*handle*/, u32 /*info*/) { return CELL_OK; }
s32 cellJpgEncEncodePicture2(u32 /*handle*/, u32 /*pic*/) { return CELL_OK; }
s32 cellJpgEncWaitForInput(u32 /*handle*/) { return CELL_OK; }
s32 cellJpgEncWaitForOutput(u32 /*handle*/) { return CELL_OK; }

}  // namespace

static hle::Module s_module_pngdec("cellPngDec", {
    HLE_FN(cellPngDecClose),
    HLE_FN(cellPngDecCreate),
    HLE_FN(cellPngDecDecodeData),
    HLE_FN(cellPngDecDestroy),
    HLE_FN(cellPngDecOpen),
    HLE_FN(cellPngDecReadHeader),
    HLE_FN(cellPngDecSetParameter),
});

static hle::Module s_module_pngenc("cellPngEnc", {
    HLE_FN(cellPngEncClose),
    HLE_FN(cellPngEncEncodePicture),
    HLE_FN(cellPngEncGetStreamInfo),
    HLE_FN(cellPngEncOpen),
    HLE_FN(cellPngEncQueryAttr),
    HLE_FN(cellPngEncWaitForInput),
    HLE_FN(cellPngEncWaitForOutput),
});

static hle::Module s_module_jpgenc("cellJpgEnc", {
    HLE_FN(cellJpgEncClose),
    HLE_FN(cellJpgEncEncodePicture2),
    HLE_FN(cellJpgEncGetStreamInfo),
    HLE_FN(cellJpgEncOpen),
    HLE_FN(cellJpgEncQueryAttr),
    HLE_FN(cellJpgEncWaitForInput),
    HLE_FN(cellJpgEncWaitForOutput),
});

}  // namespace rt
