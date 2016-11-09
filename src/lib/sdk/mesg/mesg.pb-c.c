/* Generated by the protocol buffer compiler.  DO NOT EDIT! */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C_NO_DEPRECATED
#define PROTOBUF_C_NO_DEPRECATED
#endif

#include "mesg.pb-c.h"
void   online__init
                     (Online         *message)
{
  static Online init_value = ONLINE__INIT;
  *message = init_value;
}
size_t online__get_packed_size
                     (const Online *message)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &online__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t online__pack
                     (const Online *message,
                      uint8_t       *out)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &online__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t online__pack_to_buffer
                     (const Online *message,
                      ProtobufCBuffer *buffer)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &online__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Online *
       online__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Online *)
     protobuf_c_message_unpack (&online__descriptor,
                                allocator, len, data);
}
void   online__free_unpacked
                     (Online *message,
                      ProtobufCAllocator *allocator)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &online__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   offline__init
                     (Offline         *message)
{
  static Offline init_value = OFFLINE__INIT;
  *message = init_value;
}
size_t offline__get_packed_size
                     (const Offline *message)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &offline__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t offline__pack
                     (const Offline *message,
                      uint8_t       *out)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &offline__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t offline__pack_to_buffer
                     (const Offline *message,
                      ProtobufCBuffer *buffer)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &offline__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Offline *
       offline__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Offline *)
     protobuf_c_message_unpack (&offline__descriptor,
                                allocator, len, data);
}
void   offline__free_unpacked
                     (Offline *message,
                      ProtobufCAllocator *allocator)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &offline__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   online_ack__init
                     (OnlineAck         *message)
{
  static OnlineAck init_value = ONLINE_ACK__INIT;
  *message = init_value;
}
size_t online_ack__get_packed_size
                     (const OnlineAck *message)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &online_ack__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t online_ack__pack
                     (const OnlineAck *message,
                      uint8_t       *out)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &online_ack__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t online_ack__pack_to_buffer
                     (const OnlineAck *message,
                      ProtobufCBuffer *buffer)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &online_ack__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
OnlineAck *
       online_ack__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (OnlineAck *)
     protobuf_c_message_unpack (&online_ack__descriptor,
                                allocator, len, data);
}
void   online_ack__free_unpacked
                     (OnlineAck *message,
                      ProtobufCAllocator *allocator)
{
  PROTOBUF_C_ASSERT (message->base.descriptor == &online_ack__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor online__field_descriptors[4] =
{
  {
    "cid",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    PROTOBUF_C_OFFSETOF(Online, cid),
    NULL,
    NULL,
    0,            /* packed */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "token",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    PROTOBUF_C_OFFSETOF(Online, token),
    NULL,
    NULL,
    0,            /* packed */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "version",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    PROTOBUF_C_OFFSETOF(Online, version),
    NULL,
    NULL,
    0,            /* packed */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "appkey",
    4,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    PROTOBUF_C_OFFSETOF(Online, appkey),
    NULL,
    NULL,
    0,            /* packed */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned online__field_indices_by_name[] = {
  3,   /* field[3] = appkey */
  0,   /* field[0] = cid */
  1,   /* field[1] = token */
  2,   /* field[2] = version */
};
static const ProtobufCIntRange online__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor online__descriptor =
{
  PROTOBUF_C_MESSAGE_DESCRIPTOR_MAGIC,
  "online",
  "Online",
  "Online",
  "",
  sizeof(Online),
  4,
  online__field_descriptors,
  online__field_indices_by_name,
  1,  online__number_ranges,
  (ProtobufCMessageInit) online__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor offline__field_descriptors[3] =
{
  {
    "code",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_INT32,
    PROTOBUF_C_OFFSETOF(Offline, has_code),
    PROTOBUF_C_OFFSETOF(Offline, code),
    NULL,
    NULL,
    0,            /* packed */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "msg",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    PROTOBUF_C_OFFSETOF(Offline, msg),
    NULL,
    NULL,
    0,            /* packed */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "appkey",
    3,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    PROTOBUF_C_OFFSETOF(Offline, appkey),
    NULL,
    NULL,
    0,            /* packed */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned offline__field_indices_by_name[] = {
  2,   /* field[2] = appkey */
  0,   /* field[0] = code */
  1,   /* field[1] = msg */
};
static const ProtobufCIntRange offline__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 3 }
};
const ProtobufCMessageDescriptor offline__descriptor =
{
  PROTOBUF_C_MESSAGE_DESCRIPTOR_MAGIC,
  "offline",
  "Offline",
  "Offline",
  "",
  sizeof(Offline),
  3,
  offline__field_descriptors,
  offline__field_indices_by_name,
  1,  offline__number_ranges,
  (ProtobufCMessageInit) offline__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor online_ack__field_descriptors[2] =
{
  {
    "code",
    1,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_INT32,
    PROTOBUF_C_OFFSETOF(OnlineAck, has_code),
    PROTOBUF_C_OFFSETOF(OnlineAck, code),
    NULL,
    NULL,
    0,            /* packed */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "msg",
    2,
    PROTOBUF_C_LABEL_OPTIONAL,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    PROTOBUF_C_OFFSETOF(OnlineAck, msg),
    NULL,
    NULL,
    0,            /* packed */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned online_ack__field_indices_by_name[] = {
  0,   /* field[0] = code */
  1,   /* field[1] = msg */
};
static const ProtobufCIntRange online_ack__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 2 }
};
const ProtobufCMessageDescriptor online_ack__descriptor =
{
  PROTOBUF_C_MESSAGE_DESCRIPTOR_MAGIC,
  "online_ack",
  "OnlineAck",
  "OnlineAck",
  "",
  sizeof(OnlineAck),
  2,
  online_ack__field_descriptors,
  online_ack__field_indices_by_name,
  1,  online_ack__number_ranges,
  (ProtobufCMessageInit) online_ack__init,
  NULL,NULL,NULL    /* reserved[123] */
};
