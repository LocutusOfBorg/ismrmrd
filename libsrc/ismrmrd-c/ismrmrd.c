#include <string.h>
#include <stdlib.h>

/* Language and Cross platform section for defining types */
#ifdef __cplusplus
#include <cmath>
#include <cstdio>

#else
/* C99 compiler */
#include <math.h>
#include <stdio.h>

#endif /* __cplusplus */

#include "ismrmrd/ismrmrd-c.h"
#include "ismrmrd/version.h"

#ifdef __cplusplus
namespace ISMRMRD {
extern "C" {
#endif

/* Error handling prototypes */
typedef struct ISMRMRD_error_node {
    struct ISMRMRD_error_node *next;
    char *file;
    char *func;
    char *msg;
    int line;
    int code;
} ISMRMRD_error_node_t;

static void ismrmrd_error_default(const char *file, int line,
        const char *func, int code, const char *msg);
static ISMRMRD_error_node_t *error_stack_head = NULL;
static ismrmrd_error_handler_t ismrmrd_error_handler = ismrmrd_error_default;


/* Acquisition functions */
int ismrmrd_init_acquisition_header(ISMRMRD_AcquisitionHeader *hdr) {
    if (hdr == NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
    }

    memset(hdr, 0, sizeof(ISMRMRD_AcquisitionHeader));
    hdr->version = ISMRMRD_VERSION_MAJOR;
    hdr->storage_type = ISMRMRD_FLOAT;
    hdr->number_of_samples = 0;
    hdr->available_channels = 1;
    hdr->active_channels = 1;
    return ISMRMRD_NOERROR;
}

int ismrmrd_init_acquisition(ISMRMRD_Acquisition *acq) {
    if (acq == NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
    }
    ismrmrd_init_acquisition_header(&acq->head);
    acq->traj = NULL;
    acq->data = NULL;
    return ISMRMRD_NOERROR;
}

int ismrmrd_cleanup_acquisition(ISMRMRD_Acquisition *acq) {
    if (acq == NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
    }
    
    free(acq->data);
    acq->data = NULL;
    free(acq->traj);
    acq->traj = NULL;
    return ISMRMRD_NOERROR;
}
    
ISMRMRD_Acquisition * ismrmrd_create_acquisition() {
    ISMRMRD_Acquisition *acq = (ISMRMRD_Acquisition *) malloc(sizeof(ISMRMRD_Acquisition));
    if (acq == NULL) {
        ISMRMRD_PUSH_ERR(ISMRMRD_MEMORYERROR, "Failed to malloc new ISMRMRD_Acquistion.");
        return NULL;
    }
    if (ismrmrd_init_acquisition(acq) != ISMRMRD_NOERROR)
    {
        ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Failed to initialize acquistion.");
        return NULL;
    }
    return acq;
}

int ismrmrd_free_acquisition(ISMRMRD_Acquisition *acq) {

    if (acq == NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not NULL.");
    }
    
    if (ismrmrd_cleanup_acquisition(acq)!=ISMRMRD_NOERROR) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Failed to cleanup acquisition.");
    }
    free(acq);
    return ISMRMRD_NOERROR;
}

int ismrmrd_copy_acquisition(ISMRMRD_Acquisition *acqdest, const ISMRMRD_Acquisition *acqsource) {

    if (acqsource==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Source pointer should not NULL.");
    }
    if (acqdest==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Destination pointer should not NULL.");
    }

    /* Copy the header */
    memcpy(&acqdest->head, &acqsource->head, sizeof(ISMRMRD_AcquisitionHeader));
    /* Reallocate memory for the trajectory and the data*/
    ismrmrd_make_consistent_acquisition(acqdest);
    /* Copy the trajectory and the data */
    memcpy(acqdest->traj, acqsource->traj, ismrmrd_size_of_acquisition_traj(acqsource));
    memcpy(acqdest->data, acqsource->data, ismrmrd_size_of_acquisition_data(acqsource));
    return ISMRMRD_NOERROR;
}

int ismrmrd_make_consistent_acquisition(ISMRMRD_Acquisition *acq) {

    size_t traj_size, data_size;
        
    if (acq==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not NULL.");
    }

    if (acq->head.available_channels < acq->head.active_channels) {
        acq->head.available_channels = acq->head.active_channels;
    }
    
    traj_size = ismrmrd_size_of_acquisition_traj(acq);
    if (traj_size > 0) {
        acq->traj = (float *)realloc(acq->traj, traj_size);
        if (acq->traj == NULL) {
            return ISMRMRD_PUSH_ERR(ISMRMRD_MEMORYERROR,
                          "Failed to realloc acquisition trajectory array");
        }
    }
        
    data_size = ismrmrd_size_of_acquisition_data(acq);
    if (data_size > 0) {
        acq->data = realloc(acq->data, data_size);
        if (acq->data == NULL) {
            return ISMRMRD_PUSH_ERR(ISMRMRD_MEMORYERROR,
                          "Failed to realloc acquisition data array");
        }
    }

    return ISMRMRD_NOERROR;
}

size_t ismrmrd_size_of_acquisition_traj(const ISMRMRD_Acquisition *acq) {

    int num_traj;
    
    if (acq==NULL) {
        ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not NULL.");
        return 0;
    }

    num_traj = acq->head.number_of_samples * acq->head.trajectory_dimensions;
    return num_traj * sizeof(*acq->traj);

}

size_t ismrmrd_size_of_acquisition_data(const ISMRMRD_Acquisition *acq) {
    int num_data;
    
    if (acq==NULL) {
        ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not NULL.");
        return 0;
    }

    num_data = acq->head.number_of_samples * acq->head.active_channels;
    return num_data * ismrmrd_sizeof_data_type(acq->head.storage_type);

}

/* Image functions */
int ismrmrd_init_image_header(ISMRMRD_ImageHeader *hdr) {
    if (hdr==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not NULL.");
    }
    memset(hdr, 0, sizeof(ISMRMRD_ImageHeader));
    hdr->version = ISMRMRD_VERSION_MAJOR;
    hdr->matrix_size[0] = 0;
    hdr->matrix_size[1] = 1;
    hdr->matrix_size[2] = 1;
    hdr->channels = 1;
    return ISMRMRD_NOERROR;
}

/* ImageHeader functions */
int ismrmrd_init_image(ISMRMRD_Image *im) {
    if (im==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not NULL.");
    }

    if (ismrmrd_init_image_header(&im->head) != ISMRMRD_NOERROR) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Failed to initialize image header.");
    }
    im->attribute_string = NULL;
    im->data = NULL;
    return ISMRMRD_NOERROR;
}

ISMRMRD_Image * ismrmrd_create_image() {
    ISMRMRD_Image *im = (ISMRMRD_Image *) malloc(sizeof(ISMRMRD_Image));
    if (im==NULL) {
        ISMRMRD_PUSH_ERR(ISMRMRD_MEMORYERROR, "Failed to allocate new Image.");
        return NULL;
    }
    
    if (ismrmrd_init_image(im) != ISMRMRD_NOERROR) {
        ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Failed to initialize image.");
        return NULL;
    }
    return im;
}

int ismrmrd_cleanup_image(ISMRMRD_Image *im) {
    if (im==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not NULL.");
    }
    free(im->attribute_string);
    im->attribute_string = NULL;
    free(im->data);
    im->data = NULL;
    return ISMRMRD_NOERROR;
}

int ismrmrd_free_image(ISMRMRD_Image *im) {
    if (im==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not NULL.");
    }        
    if (ismrmrd_cleanup_image(im) != ISMRMRD_NOERROR) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Failed to clean up image.");
    }
    free(im);
    return ISMRMRD_NOERROR;
}

int ismrmrd_copy_image(ISMRMRD_Image *imdest, const ISMRMRD_Image *imsource) {
    if (imsource==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Source pointer should not NULL.");
    }
    if (imdest==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Destination pointer should not NULL.");
    }
    memcpy(&imdest->head, &imsource->head, sizeof(ISMRMRD_ImageHeader));
    if (ismrmrd_make_consistent_image(imdest) != ISMRMRD_NOERROR) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Failed to make image consistent.");
    }
    memcpy(imdest->attribute_string, imsource->attribute_string,
           ismrmrd_size_of_image_attribute_string(imdest));
    memcpy(imdest->data, imsource->data, ismrmrd_size_of_image_data(imdest));
    return ISMRMRD_NOERROR;
}

int ismrmrd_make_consistent_image(ISMRMRD_Image *im) {
    size_t attr_size, data_size;
    if (im==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not NULL.");
    }
   
    attr_size = ismrmrd_size_of_image_attribute_string(im);
    if (attr_size > 0) {
        // Allocate space plus a null-terminating character
        im->attribute_string = (char *)realloc(im->attribute_string, attr_size + sizeof(*im->attribute_string));
        if (im->attribute_string == NULL) {
            return ISMRMRD_PUSH_ERR(ISMRMRD_MEMORYERROR, "Failed to realloc image attribute string");
        }
        // Set the null terminating character
        im->attribute_string[im->head.attribute_string_len] = '\0';
    }
        
    data_size = ismrmrd_size_of_image_data(im);
    if (data_size > 0) {
        im->data = realloc(im->data, data_size);
        if (im->data == NULL) {
            return ISMRMRD_PUSH_ERR(ISMRMRD_MEMORYERROR, "Failed to realloc image data array");
        }
    }
    return ISMRMRD_NOERROR;
}

size_t ismrmrd_size_of_image_data(const ISMRMRD_Image *im) {
    size_t data_size = 0;
    int num_data;
    if (im==NULL) {
        ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not NULL.");
        return 0;
    }

    if (ismrmrd_sizeof_data_type(im->head.data_type) == 0) {
        ISMRMRD_PUSH_ERR(ISMRMRD_TYPEERROR, "Invalid image data type");
        return 0;
    }

    num_data = im->head.matrix_size[0] * im->head.matrix_size[1] *
            im->head.matrix_size[2] * im->head.channels;
        
    data_size = num_data * ismrmrd_sizeof_data_type(im->head.data_type);
    
    return data_size;
}

size_t ismrmrd_size_of_image_attribute_string(const ISMRMRD_Image *im) {
    if (im==NULL) {
        ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
        return 0;
    }
    return im->head.attribute_string_len * sizeof(*im->attribute_string);
}

/* NDArray functions */
int ismrmrd_init_ndarray(ISMRMRD_NDArray *arr) {
    int n;

    if (arr==NULL) {
        ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
        return ISMRMRD_RUNTIMEERROR;
    }

    arr->version = ISMRMRD_VERSION_MAJOR;
    arr->data_type = 0; /* no default data type */
    arr->ndim = 0;
    
    for (n = 0; n < ISMRMRD_NDARRAY_MAXDIM; n++) {
        arr->dims[n] = 0;
    }
    arr->data = NULL;
    return ISMRMRD_NOERROR;
}

ISMRMRD_NDArray * ismrmrd_create_ndarray() {
    ISMRMRD_NDArray *arr = (ISMRMRD_NDArray *) malloc(sizeof(ISMRMRD_NDArray));
    if (arr==NULL) {
        ISMRMRD_PUSH_ERR(ISMRMRD_MEMORYERROR, "Failed to malloc new ISMRMRD_NDArray.");
        return NULL;
    }
        
    if (ismrmrd_init_ndarray(arr)!=ISMRMRD_NOERROR) {
        ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Failed to initialize ndarray.");
        return NULL;
    }
    return arr;
}

int ismrmrd_cleanup_ndarray(ISMRMRD_NDArray *arr) {
    if (arr==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
    }

    free(arr->data);
    arr->data = NULL;
    return ISMRMRD_NOERROR;
}

int ismrmrd_free_ndarray(ISMRMRD_NDArray *arr) {
    if (arr==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
    }

    if (ismrmrd_cleanup_ndarray(arr)!=ISMRMRD_NOERROR) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Failed to cleanup ndarray.");
    }
    free(arr);
    return ISMRMRD_NOERROR;
}

int ismrmrd_copy_ndarray(ISMRMRD_NDArray *arrdest, const ISMRMRD_NDArray *arrsource) {
    int n;

    if (arrsource==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Source pointer should not be NULL.");
    }
    if (arrdest==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Destination pointer should not be NULL.");
    }
            
    arrdest->version = arrsource->version;
    arrdest->data_type = arrsource->data_type;
    arrdest->ndim = arrsource->ndim;
            
    for (n = 0; n < ISMRMRD_NDARRAY_MAXDIM; n++) {
        arrdest->dims[n] = arrsource->dims[n];
    }
    if (ismrmrd_make_consistent_ndarray(arrdest)!=ISMRMRD_NOERROR) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Failed to make ndarray consistent.");
    }
    memcpy(arrdest->data, arrsource->data, ismrmrd_size_of_ndarray_data(arrdest));
    return ISMRMRD_NOERROR;
}

int ismrmrd_make_consistent_ndarray(ISMRMRD_NDArray *arr) {
    size_t data_size;
    
    if (arr==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
    }

    data_size = ismrmrd_size_of_ndarray_data(arr);
    if (data_size > 0) {
        arr->data = realloc(arr->data, data_size);
        if (arr->data == NULL) {
            return ISMRMRD_PUSH_ERR(ISMRMRD_MEMORYERROR, "Failed to realloc NDArray data array");
        }
    }
    return ISMRMRD_NOERROR;
}

size_t ismrmrd_size_of_ndarray_data(const ISMRMRD_NDArray *arr) {
    size_t data_size = 0;
    int num_data = 1;
    int n;
    
    if (arr==NULL) {
        ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
        return 0;
    }

    if (ismrmrd_sizeof_data_type(arr->data_type) == 0 ) {
        ISMRMRD_PUSH_ERR(ISMRMRD_TYPEERROR, "Invalid NDArray data type");
        return 0;
    }
    
    for (n = 0; n < arr->ndim; n++) {
        num_data *= (int) (arr->dims[n]);
    }

    data_size = num_data * ismrmrd_sizeof_data_type(arr->data_type);

    return data_size;
}

size_t ismrmrd_sizeof_data_type(int data_type)
{
    size_t size = 0;

    switch (data_type) {
        case ISMRMRD_USHORT:
            size = sizeof(uint16_t);
            break;
        case ISMRMRD_SHORT:
            size = sizeof(int16_t);
            break;
        case ISMRMRD_UINT:
            size = sizeof(uint32_t);
            break;
        case ISMRMRD_INT:
            size = sizeof(int32_t);
            break;
        case ISMRMRD_FLOAT:
            size = sizeof(float);
            break;
        case ISMRMRD_DOUBLE:
            size = sizeof(double);
            break;
        case ISMRMRD_CXFLOAT:
            size = sizeof(complex_float_t);
            break;
        case ISMRMRD_CXDOUBLE:
            size = sizeof(complex_double_t);
            break;
        default:
            size = 0;
    }
    return size;
}

/* Misc. functions */
bool ismrmrd_is_flag_set(const uint64_t flags, const uint64_t val) {
    uint64_t bitmask = 1 << (val - 1);
    return (flags & bitmask) > 0;
}

int ismrmrd_set_flag(uint64_t *flags, const uint64_t val) {
    uint64_t bitmask;
    if (flags==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
    }
    bitmask = 1 << (val - 1);
    *flags |= bitmask;
    return ISMRMRD_NOERROR;
}

int ismrmrd_set_flags(uint64_t *flags, const uint64_t val) {
    if (flags==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
    }
    *flags = val;
    return ISMRMRD_NOERROR;
}

int ismrmrd_clear_flag(uint64_t *flags, const uint64_t val) {
    uint64_t bitmask;
    if (flags==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
    }
    bitmask = 1 << (val - 1);
    *flags &= ~bitmask;
    return ISMRMRD_NOERROR;
}

int ismrmrd_clear_all_flags(uint64_t *flags) {
    if (flags==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer should not be NULL.");
    }
    *flags = 0;
    return ISMRMRD_NOERROR;
}

bool ismrmrd_is_channel_on(const uint64_t channel_mask[ISMRMRD_CHANNEL_MASKS], const uint16_t chan) {
    uint64_t bitmask;
    size_t offset;
    if (channel_mask==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer to channel_mask should not be NULL.");
    }
    bitmask = 1 << (chan % 64);
    offset = chan / 64;
    return (channel_mask[offset] & bitmask) > 0;
}

int ismrmrd_set_channel_on(uint64_t channel_mask[ISMRMRD_CHANNEL_MASKS], const uint16_t chan) {
    uint64_t bitmask;
    size_t offset;
    if (channel_mask==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer to channel_mask should not be NULL.");
    }
    bitmask = 1 << (chan % 64);
    offset = chan / 64;
    channel_mask[offset] |= bitmask;
    return ISMRMRD_NOERROR;
}

int ismrmrd_set_channel_off(uint64_t channel_mask[ISMRMRD_CHANNEL_MASKS], const uint16_t chan) {
    uint64_t bitmask;
    size_t offset;
    if (channel_mask==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer to channel_mask should not be NULL.");
    }
    bitmask = 1 << (chan % 64);
    offset = chan / 64;
    channel_mask[offset] &= ~bitmask;
    return ISMRMRD_NOERROR;
}
    
int ismrmrd_set_all_channels_off(uint64_t channel_mask[ISMRMRD_CHANNEL_MASKS]) {
    size_t offset;
    if (channel_mask==NULL) {
        return ISMRMRD_PUSH_ERR(ISMRMRD_RUNTIMEERROR, "Pointer to channel_mask should not be NULL.");
    }
    for (offset = 0; offset<ISMRMRD_CHANNEL_MASKS; offset++) {
        channel_mask[offset] = 0;
    }
    return ISMRMRD_NOERROR;
}

/**
 * Saves error information on the error stack
 * @returns error code
 */
int ismrmrd_push_error(const char *file, const int line, const char *func,
        const int code, const char *msg)
{
    ISMRMRD_error_node_t *node = NULL;

    /* Call user-defined error handler if it exists */
    if (ismrmrd_error_handler != NULL) {
        ismrmrd_error_handler(file, line, func, code, msg);
    }

    /* Save error information on error stack */
    node = (ISMRMRD_error_node_t*)calloc(1, sizeof(*node));
    if (node == NULL) {
        /* TODO: how to handle this? */
        return ISMRMRD_MEMORYERROR;
    }

    node->next = error_stack_head;
    error_stack_head = node;

    node->file = (char*)file;
    node->line = line;
    node->func = (char*)func;
    node->code = code;
    node->msg = (char*)msg;

    return code;
}

bool ismrmrd_pop_error(char **file, int *line, char **func,
        int *code, char **msg)
{
    ISMRMRD_error_node_t *node = error_stack_head;
    if (node == NULL) {
        /* nothing to pop */
        return false;
    }

    /* pop head off stack */
    error_stack_head = node->next;

    if (file != NULL) {
        *file = node->file;
    }
    if (line != NULL) {
        *line = node->line;
    }
    if (func != NULL) {
        *func = node->func;
    }
    if (code != NULL) {
        *code = node->code;
    }
    if (msg != NULL) {
        *msg = node->msg;
    }

    free(node);
    return true;
}

void ismrmrd_set_error_handler(ismrmrd_error_handler_t handler) {
    ismrmrd_error_handler = handler;
}

char *ismrmrd_strerror(int code) {
    /* Match the ISMRMRD_ErrorCodes */
    static char * const error_messages []= {
        "No Error",
        "Memory Error",
        "File Error",
        "Type Error",
        "Runtime Error",
        "HDF5 Error",
    };
    return error_messages[code];
}

static void ismrmrd_error_default(const char *file, int line,
        const char *func, int code, const char *msg)
{
    char *msgtype = ismrmrd_strerror(code);
    fprintf(stderr, "ERROR: %s in %s, line %d: %s\n", msgtype, file, line, msg);
}

#ifdef __cplusplus
} // extern "C"
} // namespace ISMRMRD
#endif