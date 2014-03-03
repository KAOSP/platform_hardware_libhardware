/*
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_TV_INPUT_INTERFACE_H
#define ANDROID_TV_INPUT_INTERFACE_H

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <hardware/hardware.h>

__BEGIN_DECLS

/*
 * Module versioning information for the TV input hardware module, based on
 * tv_input_module_t.common.module_api_version.
 *
 * Version History:
 *
 * TV_INPUT_MODULE_API_VERSION_0_1:
 * Initial TV input hardware module API.
 *
 */

#define TV_INPUT_MODULE_API_VERSION_0_1  HARDWARE_MODULE_API_VERSION(0, 1)

#define TV_INPUT_DEVICE_API_VERSION_0_1  HARDWARE_DEVICE_API_VERSION(0, 1)

/*
 * The id of this module
 */
#define TV_INPUT_HARDWARE_MODULE_ID "tv_input"

#define TV_INPUT_DEFAULT_DEVICE "default"

/*****************************************************************************/

/*
 * Every hardware module must have a data structure named HAL_MODULE_INFO_SYM
 * and the fields of this data structure must begin with hw_module_t
 * followed by module specific information.
 */
typedef struct tv_input_module {
    struct hw_module_t common;
} tv_input_module_t;

/*****************************************************************************/

typedef enum tv_input_type {
    /* HDMI */
    TV_INPUT_TYPE_HDMI = 1,

    /* Built-in tuners. */
    TV_INPUT_TYPE_BUILT_IN_TUNER = 2,

    /* Passthrough */
    TV_INPUT_TYPE_PASSTHROUGH = 3,
} tv_input_type_t;

typedef struct tv_input_device_info {
    /* Device ID */
    int device_id;

    /* Type of physical TV input. */
    tv_input_type_t type;

    /*
     * TODO: A union of type specific information. For example, HDMI port
     * identifier that HDMI hardware understands.
     */

    /* TODO: Add capability if necessary. */

    /* TODO: Audio info */
} tv_input_device_info_t;

typedef enum {
    /*
     * Hardware notifies the framework that a device is available.
     */
    TV_INPUT_EVENT_DEVICE_AVAILABLE = 1,
    /*
     * Hardware notifies the framework that a device is unavailable.
     */
    TV_INPUT_EVENT_DEVICE_UNAVAILABLE = 2,
    /*
     * Stream configurations are changed. Client should regard all open streams
     * at the specific device are closed, and should call
     * get_stream_configurations() again, opening some of them if necessary.
     */
    TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED = 3,
    /* TODO: Buffer notifications, etc. */
} tv_input_event_type_t;

typedef struct tv_input_event {
    tv_input_event_type_t type;

    union {
        /*
         * TV_INPUT_EVENT_DEVICE_AVAILABLE: all fields are relevant
         * TV_INPUT_EVENT_DEVICE_UNAVAILABLE: only device_id is relevant
         * TV_INPUT_EVENT_STREAM_CONFIGURATIONS_CHANGED: only device_id is
         *    relevant
         */
        tv_input_device_info_t device_info;
    };
} tv_input_event_t;

typedef struct tv_input_callback_ops {
    /*
     * event contains the type of the event and additional data if necessary.
     * The event object is guaranteed to be valid only for the duration of the
     * call.
     *
     * data is an object supplied at device initialization, opaque to the
     * hardware.
     */
    void (*notify)(struct tv_input_device* dev,
            tv_input_event_t* event, void* data);
} tv_input_callback_ops_t;

typedef enum {
    TV_STREAM_TYPE_INDEPENDENT_VIDEO_SOURCE = 1,
    /* TODO: TV_STREAM_TYPE_BUFFER_PRODUCER = 2, */
} tv_stream_type_t;

typedef struct tv_stream_config {
    /*
     * ID number of the stream. This value is used to identify the whole stream
     * configuration.
     */
    int stream_id;

    /* Type of the stream */
    tv_stream_type_t type;

    /* Max width/height of the stream. */
    uint32_t max_video_width;
    uint32_t max_video_height;
} tv_stream_config_t;

typedef struct tv_stream {
    /* IN: ID in the stream configuration.  */
    int stream_id;

    /* OUT: Type of the stream (for convenience) */
    tv_stream_type_t type;

    /* OUT: Data associated with the stream for client's use */
    union {
        native_handle_t* sideband_stream_source_handle;
        /* TODO: buffer_producer_stream_t buffer_producer; */
    };
} tv_stream_t;

/*
 * Every device data structure must begin with hw_device_t
 * followed by module specific public methods and attributes.
 */
typedef struct tv_input_device {
    struct hw_device_t common;

    /*
     * initialize:
     *
     * Provide callbacks to the device and start operation. At first, no device
     * is available and after initialize() completes, currently available
     * devices including static devices should notify via callback.
     *
     * Framework owns callbacks object.
     *
     * data is a framework-owned object which would be sent back to the
     * framework for each callback notifications.
     *
     * Return 0 on success.
     */
    int (*initialize)(struct tv_input_device* dev,
            const tv_input_callback_ops_t* callback, void* data);

    /*
     * get_stream_configurations:
     *
     * Get stream configurations for a specific device. An input device may have
     * multiple configurations.
     *
     * The configs object is guaranteed to be valid only until the next call to
     * get_stream_configurations() or STREAM_CONFIGURATIONS_CHANGED event.
     *
     * Return 0 on success.
     */
    int (*get_stream_configurations)(const struct tv_input_device* dev,
            int device_id, int* num_configurations,
            const tv_stream_config_t** configs);

    /*
     * open_stream:
     *
     * Open a stream with given stream ID. Caller owns stream object, and the
     * populated data is only valid until the stream is closed.
     *
     * Return 0 on success; -EBUSY if the client should close other streams to
     * open the stream; -EEXIST if the stream with the given ID is already open;
     * -EINVAL if device_id and/or stream_id are invalid; other non-zero value
     * denotes unknown error.
     */
    int (*open_stream)(struct tv_input_device* dev, int device_id,
            tv_stream_t* stream);

    /*
     * close_stream:
     *
     * Close a stream to a device. data in tv_stream_t* object associated with
     * the stream_id is obsolete once this call finishes.
     *
     * Return 0 on success; -ENOENT if the stream is not open; -EINVAL if
     * device_id and/or stream_id are invalid.
     */
    int (*close_stream)(struct tv_input_device* dev, int device_id,
            int stream_id);

    /*
     * TODO: Add more APIs such as buffer operations in case of buffer producer
     * profile.
     */

    void* reserved[16];
} tv_input_device_t;

__END_DECLS

#endif  // ANDROID_TV_INPUT_INTERFACE_H
