#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#include "fpdfview.h"
#include "fpdf_dataavail.h"

#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE void PDFium_Init()
{
    // https://source.chromium.org/chromium/chromium/src/+/master:third_party/pdfium/samples/pdfium_test.cc;l=1172

    FPDF_LIBRARY_CONFIG config;
    config.version = 3;
    config.m_pUserFontPaths = NULL;
    config.m_pIsolate = NULL;
    config.m_v8EmbedderSlot = 0;
    config.m_pPlatform = NULL;

    FPDF_InitLibraryWithConfig(&config);
}

EMSCRIPTEN_KEEPALIVE bool PDFium_CheckDimensions(int stride, int width, int height)
{
    if (stride < 0 || width < 0 || height < 0)
    {
        return false;
    }

    if (height > 0 && stride > INT_MAX / height)
    {
        return false;
    }

    return true;
}

EMSCRIPTEN_KEEPALIVE FPDF_DOCUMENT PDFium_CreateDocFromBuffer(void *buffer, int length)
{
    return FPDF_LoadMemDocument(buffer, length, "");
}

EMSCRIPTEN_KEEPALIVE double *PDFium_GetPageSizeByIndex(FPDF_DOCUMENT doc, int pageIndex)
{
    double *result = malloc(2 * sizeof(double));

    double pageWidth;
    double pageHeight;

    FPDF_GetPageSizeByIndex(doc, pageIndex, &pageWidth, &pageHeight);

    pageWidth = pageWidth * 1.333333;
    pageHeight = pageHeight * 1.333333;

    result[0] = pageWidth;
    result[1] = pageHeight;

    return result;
}

EMSCRIPTEN_KEEPALIVE char *PDFium_GetRenderPageBitmap(FPDF_DOCUMENT doc, int pageIndex, bool reverseByteOrder)
{
    // page size
    double pageWidth;
    double pageHeight;

    FPDF_GetPageSizeByIndex(doc, pageIndex, &pageWidth, &pageHeight);

    pageWidth = pageWidth * 1.333333;
    pageHeight = pageHeight * 1.333333;

    // render page
    FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);

    // get alpha
    //int alpha = FPDFPage_HasTransparency(page) ? 1 : 0;   // C++ only
    int alpha = 1;

    // create bitmap
    FPDF_BITMAP pageBitmap = FPDFBitmap_Create((int)pageWidth, (int)pageHeight, alpha);
    FPDF_DWORD fillColor = alpha ? 0x00000000 : 0xFFFFFFFF;

    FPDFBitmap_FillRect(pageBitmap, 0, 0, (int)pageWidth, (int)pageHeight, fillColor);

    int flags = 0;

    if (reverseByteOrder)
    {
        flags = FPDF_ANNOT | FPDF_PRINTING | FPDF_REVERSE_BYTE_ORDER;
    }
    else
    {
        flags = FPDF_ANNOT | FPDF_PRINTING;
    }

    FPDF_RenderPageBitmap(pageBitmap, page, 0, 0, (int)pageWidth, (int)pageHeight, 0, flags);

    int stride = FPDFBitmap_GetStride(pageBitmap);
    char *buffer = FPDFBitmap_GetBuffer(pageBitmap);

    FPDFBitmap_Destroy(pageBitmap);
    FPDF_ClosePage(page);

    return buffer;
}

EMSCRIPTEN_KEEPALIVE int PDFium_GetRenderPageDataSize(FPDF_DOCUMENT doc, int pageIndex)
{
    // page size
    double pageWidth;
    double pageHeight;

    FPDF_GetPageSizeByIndex(doc, pageIndex, &pageWidth, &pageHeight);

    pageWidth = pageWidth * 1.333333;
    pageHeight = pageHeight * 1.333333;

    return (int)((int)pageWidth * (int)pageHeight * 4);
}

EMSCRIPTEN_KEEPALIVE char *PDFium_GetLastError()
{
    char *result = "";

    unsigned long err = FPDF_GetLastError();

    switch (err)
    {
    case FPDF_ERR_SUCCESS:
        result = "success";
        break;
    case FPDF_ERR_UNKNOWN:
        result = "unknown error";
        break;
    case FPDF_ERR_FILE:
        result = "file not found or could not be opened";
        break;
    case FPDF_ERR_FORMAT:
        result = "file not in PDF format or corrupted";
        break;
    case FPDF_ERR_PASSWORD:
        result = "password required or incorrect password";
        break;
    case FPDF_ERR_SECURITY:
        result = "unsupported security scheme";
        break;
    case FPDF_ERR_PAGE:
        result = "page not found or content error";
        break;
    default:
        result = "unknown error";
    }

    return result;
}
