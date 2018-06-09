/*******************************************************************************
 * Copyright (c) 2016  MSc. David Graeff <david.graeff@web.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#include <gtest/gtest.h>
#include "lwm2m_connect.h"
#include "lwm2m_objects.h"
#include "lwm2m_objects.hpp"
#include "client_debug.h"
#include "network.h"
#include <stdint.h>
#include "test_object_c.h"
#include "test_object_cpp.h"
#include "memory.h"

#include "lwm2mObjects/known.h"

using namespace KnownObjects;

#include <memory>

extern "C" {
#include "internals.h"
}

bool executed = false;
char writeFunStrResult[100];
int writeLenResult = 0;

void executeFun(uint8_t *,
                int) {
    executed = true;
}

void writeFunStr(const char* data, int len){
    writeLenResult = len;
    strncpy(writeFunStrResult,data,100);
}

void writeFunStrCPP(DynArray<const char*>& data){
    writeLenResult = data.len;
    strncpy(writeFunStrResult,data.data,100);
}

int8_t readFun8() {
    return -12;
}

uint32_t readFun32() {
    return 0xFFFF;
}

const char* readFunStr() {
    return "testR";
}

void Reading(lwm2m_context_t * lwm2mH) {
    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID, 1024, 10, 0};
    lwm2m_media_type_t format = LWM2M_CONTENT_JSON;
    std::string fullRead;
    {
        size_t buffer_len;
        char* buffer=nullptr;
        uint8_t s = object_read(lwm2mH,&uri,&format,(uint8_t**)&buffer,&buffer_len);
        ASSERT_EQ(s, CONTENT_2_05);
        fullRead.assign(buffer,buffer_len);
        lwm2m_free(buffer);
    }

    const char expect[] = "{\"bn\":\"/1024/10/\",\"e\":[{\"n\":\"0\",\"v\":15},{\"n\":\"1\",\"v\":-15},{\"n\":\"2\",\"v\":4095},{\"n\":\"3\",\"v\":-4095},{\"n\":\"4\",\"v\":268435455},{\"n\":\"5\",\"v\":-268435455},{\"n\":\"6\",\"v\":-281474976710655},{\"n\":\"7\",\"v\":0.12},{\"n\":\"8\",\"bv\":true},{\"n\":\"9\",\"sv\":\"test\"},{\"n\":\"10\",\"sv\":\"test\"},{\"n\":\"11\",\"sv\":\"dGVzdA==\"},{\"n\":\"12\",\"sv\":\"YWIACmFi\"},{\"n\":\"14\",\"v\":-12},{\"n\":\"15\",\"v\":65535},{\"n\":\"16\",\"sv\":\"testR\"},{\"n\":\"18\",\"sv\":\"testR\"},{\"n\":\"19\",\"sv\":\"\"}]}";

    ASSERT_STREQ(expect, fullRead.c_str());
    ASSERT_EQ(sizeof(expect),fullRead.size()+1);
}

void ExecutingNonExecutable(lwm2m_context_t * lwm2mH) {
    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID, 1024, 10, 0};
    uint8_t s = object_execute(lwm2mH,&uri,nullptr, 0);
    ASSERT_EQ(s, COAP_405_METHOD_NOT_ALLOWED);
}

void ExecutingFunction(lwm2m_context_t * lwm2mH){
    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID|LWM2M_URI_FLAG_RESOURCE_ID, 1024, 10, 13};
    executed = false;
    uint8_t s = object_execute(lwm2mH,&uri,nullptr, 0);
    ASSERT_EQ(s, COAP_204_CHANGED);
    ASSERT_TRUE(executed);
}

void Discover(lwm2m_context_t * lwm2mH){
#define LWM2M_SERVER_ADDR "coap://127.0.0.1"
    ASSERT_TRUE(lwm2m_add_server(123, LWM2M_SERVER_ADDR, 100, false));

    lwm2m_server_t  server;
    server.next = 0;
    server.secObjInstID = 0;
    server.shortID = 1;
    server.registration = 1;
    server.binding = BINDING_U;
    server.sessionH = lwm2mH;
    server.status = STATE_REGISTERED;

    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID, 1024, 10, 0};

    std::string fullRead;
    {
        size_t buffer_len;
        char* buffer=nullptr;
        uint8_t s = object_discover(lwm2mH,&uri,&server,(uint8_t**)&buffer, &buffer_len);
        ASSERT_EQ(s, CONTENT_2_05);
        fullRead.assign(buffer,buffer_len);
        lwm2m_free(buffer);
    }

    const char expect[] = "</1024/10>,</1024/10/0>,</1024/10/1>,</1024/10/2>,</1024/10/3>,</1024/10/4>,</1024/10/5>,</1024/10/6>,</1024/10/7>,</1024/10/8>,</1024/10/9>,</1024/10/10>,</1024/10/11>,</1024/10/12>,</1024/10/13>,</1024/10/14>,</1024/10/15>,</1024/10/16>,</1024/10/17>,</1024/10/18>,</1024/10/19>";
    ASSERT_STREQ(expect, fullRead.c_str());
    ASSERT_EQ(sizeof(expect),fullRead.size()+1);
}

template<class T>
void WriteIntBoolDouble(lwm2m_context_t * lwm2mH, lwm2m_object_t* test_object){
    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID|LWM2M_URI_FLAG_RESOURCE_ID, 1024, 10, 0};
    const char* valueInt = "10"; size_t len = strlen(valueInt);
    T * targetP = (T *)test_object->instanceList;
    uint8_t s;

    // Check number writing

    for (int i = 0; i < 7; ++i)
    {
        uri.resourceId = i;
        lwm2m_attributes_t attrP;
        attrP.toSet = ATTR_FLAG_NUMERIC;
        ASSERT_EQ(COAP_205_CONTENT, object_checkReadable(lwm2mH,&uri,&attrP));
        s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)valueInt, len);
        ASSERT_EQ(s, CHANGED_2_04);
    }

    ASSERT_EQ(targetP->test_i8, 10);
    ASSERT_EQ(targetP->test_u8, 10);
    ASSERT_EQ(targetP->test_i16, 10);
    ASSERT_EQ(targetP->test_u16, 10);
    ASSERT_EQ(targetP->test_i32, 10);
    ASSERT_EQ(targetP->test_u32, 10);
    ASSERT_EQ(targetP->test_i64, 10);

    // check double writing

    uri.resourceId = 7;
    s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)valueInt, len);
    ASSERT_EQ(s, COAP_400_BAD_REQUEST) << "Verify method does not work";

    const char* doubleValue = "0.8"; len = strlen(doubleValue);
    s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)doubleValue, len);
    ASSERT_EQ(COAP_204_CHANGED, s) << "Verify method does not work";
    ASSERT_EQ(targetP->test_double, 0.8);

    // check boolean writing

    targetP->test_bool = false;
    const char* boolValue = "1"; len = strlen(boolValue);
    uri.resourceId = 8;
    s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)boolValue, len);
    ASSERT_EQ(COAP_204_CHANGED, s);
    ASSERT_EQ(targetP->test_bool, true);
}

void WriteToReadOnlyString(lwm2m_context_t * lwm2mH){
    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID|LWM2M_URI_FLAG_RESOURCE_ID, 1024, 10, 9};
    const char* value = "newcontent"; size_t len = strlen(value);

    uint8_t s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)value, len);
    ASSERT_EQ(COAP_405_METHOD_NOT_ALLOWED, s);
}

template<class T>
void WriteStringPrealloc(lwm2m_context_t * lwm2mH, lwm2m_object_t* test_object){
    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID|LWM2M_URI_FLAG_RESOURCE_ID, 1024, 10, 10};
    const char* value = "newcontent"; size_t len = strlen(value)+1;
    T * targetP = (T *)test_object->instanceList;
    uint8_t s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)value, len);
    ASSERT_EQ(COAP_204_CHANGED, s);
    ASSERT_STREQ(value, (char*)targetP->test_str_prealloc.data);
}

void WriteStringIndirect(lwm2m_context_t * lwm2mH){
    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID|LWM2M_URI_FLAG_RESOURCE_ID, 1024, 10, 17};
    const char* value = "newcontent"; size_t len = strlen(value)+1;
    uint8_t s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)value, len);
    ASSERT_EQ(COAP_204_CHANGED, s);
    ASSERT_STREQ(value, writeFunStrResult);
}

void ReadWriteStringIndirect(lwm2m_context_t * lwm2mH){
    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID|LWM2M_URI_FLAG_RESOURCE_ID, 1024, 10, 18};
    const char* value = "newcontent"; size_t len = strlen(value)+1;
    uint8_t s;
    std::string fullRead;
    {
        lwm2m_media_type_t format = LWM2M_CONTENT_TEXT;
        size_t buffer_len;
        char* buffer=nullptr;
        s = object_read(lwm2mH,&uri,&format,(uint8_t**)&buffer, &buffer_len);
        ASSERT_EQ(COAP_205_CONTENT, s);
        fullRead.assign(buffer,buffer_len);
        lwm2m_free(buffer);
    }
    ASSERT_STREQ("testR", fullRead.c_str());

    s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)value, len);
    ASSERT_EQ(COAP_204_CHANGED, s);
    ASSERT_STREQ(value, writeFunStrResult);
}

void WriteTooBigOpaque(lwm2m_context_t * lwm2mH, lwm2m_object_t* test_object){
    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID|LWM2M_URI_FLAG_RESOURCE_ID, 1024, 10, 11};
    const char* value = "newcontent"; size_t len = strlen(value);
    uint8_t s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)value, len);
    ASSERT_EQ(COAP_413_ENTITY_TOO_LARGE, s);
}

template<class T>
void WriteOpaquePreAlloc(lwm2m_context_t * lwm2mH, lwm2m_object_t* test_object){
    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID|LWM2M_URI_FLAG_RESOURCE_ID, 1024, 10, 12};
    const char* value = "newcontent"; size_t len = 11;
    T * targetP = (T *)test_object->instanceList;
    uint8_t s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)value, len);
    ASSERT_EQ(COAP_204_CHANGED, s);
    ASSERT_EQ(len, targetP->test_opaque_prealloc.used_len);
    ASSERT_STREQ(value, (char*)targetP->test_opaque_prealloc.data);

    // Test derived opaque object
    uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID|LWM2M_URI_FLAG_RESOURCE_ID, 1024, 10, 19};
    s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)value, len);
    ASSERT_EQ(COAP_204_CHANGED, s);
    ASSERT_EQ(len, targetP->test_opaque_prealloc.used_len);
    ASSERT_STREQ(value, (char*)targetP->test_opaque_prealloc.data);
}

template<class T>
void WriteOpaqueIndirect(lwm2m_context_t * lwm2mH, lwm2m_object_t* test_object){
    lwm2m_uri_t uri = {LWM2M_URI_FLAG_OBJECT_ID|LWM2M_URI_FLAG_INSTANCE_ID|LWM2M_URI_FLAG_RESOURCE_ID, 1024, 10, 11};
    const char* value = "newcontent"; size_t len = strlen(value);
    T * targetP = (T *)test_object->instanceList;

    // Modify memory area for the test
    uint8_t buffer[10];
    targetP->test_opaque.data = buffer;
    targetP->test_opaque.reserved_len = 10;
    targetP->test_opaque.used_len = 0;

    uint8_t s = object_write(lwm2mH,&uri,LWM2M_CONTENT_TEXT, (uint8_t*)value, len);
    ASSERT_EQ(COAP_204_CHANGED, s);
    ASSERT_EQ(len, targetP->test_opaque.used_len);
    ASSERT_TRUE(memcmp(value, (char*)targetP->test_opaque.data,len)==0);
}

class ObjectAPI_C : public testing::Test {
public:
    lwm2m_context_t * lwm2mH;
    lwm2m_object_t* test_object;
protected:
    virtual void TearDown() {
        lwm2m_object_instance_remove(lwm2mH, test_object, 10);
        lwm2m_client_close();
        lwm2mH = nullptr;
        ASSERT_STREQ("", memoryObserver.printIfNotEmpty().c_str());
    }

    virtual void SetUp() {
        memoryObserver.reset();
        executed = false;
        memset(writeFunStrResult,0,sizeof(writeFunStrResult));

        lwm2mH = lwm2m_client_init("testClient");
        ASSERT_TRUE(lwm2mH) << "Failed to initialize wakaama\r\n";

        test_object = get_test_object();
        ASSERT_EQ(COAP_NO_ERROR, lwm2m_add_initialize_object(lwm2mH, test_object, true));
        auto instance = test_object_create_instance(10,
                                                    readFun8,
                                                    readFun32,
                                                    readFunStr,
                                                    writeFunStr,
                                                    executeFun);
        ASSERT_EQ(COAP_NO_ERROR, lwm2m_object_instance_add(lwm2mH, test_object, instance));

        lwm2m_object_t* o = (lwm2m_object_t*)lwm2m_list_find((lwm2m_list_t *)lwm2mH->objectList, 1024);
        ASSERT_EQ(o, test_object);


        ASSERT_TRUE(object_isInstanceNew(lwm2mH, 1024, 0));
        ASSERT_FALSE(object_isInstanceNew(lwm2mH, 1024, 10));
    }
};

TEST_F(ObjectAPI_C, Reading) { Reading(lwm2mH); }
TEST_F(ObjectAPI_C, ExecutingNonExecutable) { ExecutingNonExecutable(lwm2mH); }
TEST_F(ObjectAPI_C, ExecutingFunction) { ExecutingFunction(lwm2mH); }
TEST_F(ObjectAPI_C, Discover) { Discover(lwm2mH); }
TEST_F(ObjectAPI_C, WriteIntBoolDouble) { WriteIntBoolDouble<test_object_instance_t>(lwm2mH, test_object); }
TEST_F(ObjectAPI_C, WriteToReadOnlyString) { WriteToReadOnlyString(lwm2mH); }
TEST_F(ObjectAPI_C, WriteStringPrealloc) { WriteStringPrealloc<test_object_instance_t>(lwm2mH, test_object); }
TEST_F(ObjectAPI_C, WriteTooBigOpaque) { WriteTooBigOpaque(lwm2mH, test_object); }
TEST_F(ObjectAPI_C, WriteOpaquePreAlloc) { WriteOpaquePreAlloc<test_object_instance_t>(lwm2mH, test_object); }
TEST_F(ObjectAPI_C, WriteOpaqueIndirect) { WriteOpaqueIndirect<test_object_instance_t>(lwm2mH, test_object); }


class ObjectAPI_CPP : public testing::Test {
public:
    lwm2m_context_t * lwm2mH;
    MyTestObjectInstance instance;
    MyTestObject object;
protected:
    virtual void TearDown() {
        object.removeInstance(lwm2mH, 10);
        lwm2m_client_close();
        lwm2mH = nullptr;
        ASSERT_STREQ("", memoryObserver.printIfNotEmpty().c_str());
    }

    virtual void SetUp() {
        memoryObserver.reset();
        executed = false;
        memset(writeFunStrResult,0,sizeof(writeFunStrResult));

        lwm2mH = lwm2m_client_init("testClient");
        ASSERT_TRUE(lwm2mH) << "Failed to initialize wakaama\r\n";

        MyTestObjectSecond s;
        ASSERT_EQ(7, s.resources_len());
        ASSERT_EQ(20, object.resources_len());

        instance = MyTestObjectInstance( executeFun,
                                         readFun8,
                                         readFun32,
                                         readFunStr,
                                         writeFunStrCPP,
                                         IndirectReadWrite<const char*>(readFunStr,writeFunStrCPP));
        instance.id=10;
        object.addInstance(lwm2mH, &instance);
        object.registerObject(lwm2mH, false);

        lwm2m_object_t* o = (lwm2m_object_t*)lwm2m_list_find((lwm2m_list_t *)lwm2mH->objectList, 1024);
        ASSERT_EQ(o, &object.object);

        ASSERT_TRUE(object_isInstanceNew(lwm2mH, 1024, 0));
        ASSERT_FALSE(object_isInstanceNew(lwm2mH, 1024, 10));
    }
};

TEST_F(ObjectAPI_CPP, KnownObject3311) {
    id3311::object o;
    id3311::instance i;
    i.id = 0;
    o.addInstance(lwm2mH,&i);
    o.registerObject(lwm2mH, false);
    ASSERT_FALSE(object_isInstanceNew(lwm2mH, 3311, 0));

}

TEST_F(ObjectAPI_CPP, Reading) { Reading(lwm2mH); }
TEST_F(ObjectAPI_CPP, ExecutingNonExecutable) { ExecutingNonExecutable(lwm2mH); }
TEST_F(ObjectAPI_CPP, ExecutingFunction) { ExecutingFunction(lwm2mH); }
TEST_F(ObjectAPI_CPP, Discover) { Discover(lwm2mH); }
TEST_F(ObjectAPI_CPP, WriteIntBoolDouble) { WriteIntBoolDouble<MyTestObjectInstance>(lwm2mH, &object.object); }
TEST_F(ObjectAPI_CPP, WriteToReadOnlyString) { WriteToReadOnlyString(lwm2mH); }
TEST_F(ObjectAPI_CPP, WriteStringPrealloc) { WriteStringPrealloc<MyTestObjectInstance>(lwm2mH, &object.object); }
TEST_F(ObjectAPI_CPP, WriteTooBigOpaque) { WriteTooBigOpaque(lwm2mH, &object.object); }
TEST_F(ObjectAPI_CPP, WriteOpaquePreAlloc) { WriteOpaquePreAlloc<MyTestObjectInstance>(lwm2mH, &object.object); }
TEST_F(ObjectAPI_CPP, WriteOpaqueIndirect) { WriteOpaqueIndirect<MyTestObjectInstance>(lwm2mH, &object.object); }
TEST_F(ObjectAPI_CPP, WriteStringIndirect) { WriteStringIndirect(lwm2mH); }
TEST_F(ObjectAPI_CPP, ReadWriteStringIndirect) { ReadWriteStringIndirect(lwm2mH); }