//
//  objv_json.c
//  objv
//
//  Created by zhang hailong on 14-2-3.
//  Copyright (c) 2014年 hailong.org. All rights reserved.
//

#include "objv_os.h"
#include "objv_json.h"
#include "objv_autorelease.h"
#include "objv_value.h"
#include "objv_array.h"
#include "objv_dictionary.h"

void objv_json_encode_mbuf(objv_zone_t * zone, objv_object_t * object,objv_mbuf_t * mbuf){
    
    if(object == NULL){
        objv_mbuf_format(mbuf,"null");
    }
    else if(objv_object_isKindOfClass(object, & objv_array_class)){
        
        {
            objv_array_t * a = (objv_array_t *) object;
            int i;
            
            objv_mbuf_append(mbuf,"[",1);
        
            for(i = 0;i< a->length;i++){
                if( i != 0){
                    objv_mbuf_append(mbuf,",",1);
                }
                objv_json_encode_mbuf(zone,objv_array_objectAt(a,i),mbuf);
            }
            
            objv_mbuf_append(mbuf,"]",1);
        }
    }
    else if(objv_object_isKindOfClass(object, & objv_dictionary_class)){
        
        {
            objv_dictionary_t * d = (objv_dictionary_t *) object;
            objv_string_t * key;
            objv_object_t * v;
            int i,c = objv_dictionary_length(d);
            
            objv_mbuf_append(mbuf,"{",1);
        
            for (i = 0; i < c; i ++) {
                
                if( i != 0){
                    objv_mbuf_append(mbuf,",",1);
                }
                
                key = objv_object_stringValue(objv_dictionary_keyAt(d,i),NULL);
                
                objv_json_encode_mbuf(zone,(objv_object_t *) key,mbuf);
                
                objv_mbuf_append(mbuf,":",1);
                
                v = objv_dictionary_valueAt(d,i);
                
                objv_json_encode_mbuf(zone,v,mbuf);
            }
            
            objv_mbuf_append(mbuf,"}",1);
        }
        
    }
    else if(objv_object_isKindOfClass(object, & objv_value_class)){
        
        {
            objv_value_t * v = (objv_value_t *) object;
            if(v->type == & objv_type_int){
                objv_mbuf_format(mbuf,"%d",v->intValue);
            }
            else if(v->type == & objv_type_uint){
                objv_mbuf_format(mbuf,"%u",v->uintValue);
            }
            else if(v->type == & objv_type_long){
                objv_mbuf_format(mbuf,"%ld",v->longValue);
            }
            else if(v->type == & objv_type_ulong){
                objv_mbuf_format(mbuf,"%lu",v->ulongValue);
            }
            else if(v->type == & objv_type_longLong){
                objv_mbuf_format(mbuf,"%lld",v->longLongValue);
            }
            else if(v->type == & objv_type_ulongLong){
                objv_mbuf_format(mbuf,"%llu",v->ulongLongValue);
            }
            else if(v->type == & objv_type_float){
                objv_mbuf_format(mbuf,"%f",(double) v->floatValue);
            }
            else if(v->type == & objv_type_double){
                objv_mbuf_format(mbuf,"%lf",v->doubleValue);
            }
            else if(v->type == & objv_type_boolean){
                if(v->booleanValue){
                    objv_mbuf_append(mbuf,"true",4);
                }
                else{
                    objv_mbuf_append(mbuf,"false",5);
                }
            }
            else{
                {
                    size_t size = v->type->size;
                    unsigned char * p = v->ptrValue;
                    
                    objv_mbuf_append(mbuf,"\"",1);
                
                    while(p && size >0 ){
                        
                        objv_mbuf_format(mbuf,"%02x", *p);
                        
                        p ++;
                        size --;
                    }
                    
                    objv_mbuf_append(mbuf,"\"",1);
                }
            }
        }
    }
    else if(objv_object_isKindOfClass(object, & objv_string_class)){
        {
            objv_string_t * s = (objv_string_t *) object;
            if(s){
                char * p = (char *) s->UTF8String;
                size_t len = s->length;
                
                objv_mbuf_append(mbuf,"\"",1);
                
                while(p && *p != 0 && len >0){
                    
                    if(*p == '\n'){
                        objv_mbuf_append(mbuf,"\\n",2);
                    }
                    else if(*p == '\r'){
                        objv_mbuf_append(mbuf,"\\r",2);
                    }
                    else if(*p == '\t'){
                        objv_mbuf_append(mbuf,"\\t",2);
                    }
                    else if(*p == '\\'){
                        objv_mbuf_append(mbuf,"\\\\",2);
                    }
                    else if(*p == '"'){
                        objv_mbuf_append(mbuf,"\\\"",2);
                    }
                    else {
                        objv_mbuf_append(mbuf,p,1);
                    }
                    p ++;
                    len --;
                }
                
                objv_mbuf_append(mbuf,"\"",1);
                
            }
            else{
                objv_mbuf_format(mbuf,"null");
            }
        }
    }
    else {
        {
            objv_property_t * p;
            objv_class_t * clazz = object->isa;
            int c,i = 0;
    
            objv_mbuf_append(mbuf,"{",1);
            
            while(clazz){
                
                p = clazz->propertys;
                c = clazz->propertyCount;
                
                while(p && c > 0){
                    
                    if(p->getter){
                        
                        if(p->type == & objv_type_int){
                            
                            if( i != 0){
                                objv_mbuf_append(mbuf,",",1);
                            }
                            
                            objv_mbuf_format(mbuf,"\"%s\":",p->name->name);
                            
                            objv_mbuf_format(mbuf,"%d", (* (objv_object_property_intValue_t)p->getter->impl)(clazz,object));
                            
                            i ++;
                        }
                        else if(p->type == & objv_type_uint){
                            
                            if( i != 0){
                                objv_mbuf_append(mbuf,",",1);
                            }
                            
                            objv_mbuf_format(mbuf,"\"%s\":",p->name->name);
                            
                            objv_mbuf_format(mbuf,"%u", (* (objv_object_property_uintValue_t)p->getter->impl)(clazz,object));
                            
                            i ++;
                            
                        }
                        else if(p->type == & objv_type_long){
                            if( i != 0){
                                objv_mbuf_append(mbuf,",",1);
                            }
                            
                            objv_mbuf_format(mbuf,"\"%s\":",p->name->name);
                            
                            objv_mbuf_format(mbuf,"%ld", (* (objv_object_property_longValue_t)p->getter->impl)(clazz,object));
                            
                            i ++;
                        }
                        else if(p->type == & objv_type_ulong){
                            if( i != 0){
                                objv_mbuf_append(mbuf,",",1);
                            }
                            
                            objv_mbuf_format(mbuf,"\"%s\":",p->name->name);
                            
                            objv_mbuf_format(mbuf,"%lu", (* (objv_object_property_ulongValue_t)p->getter->impl)(clazz,object));
                            
                            i ++;
                        }
                        else if(p->type == & objv_type_longLong){
                            if( i != 0){
                                objv_mbuf_append(mbuf,",",1);
                            }
                            
                            objv_mbuf_format(mbuf,"\"%s\":",p->name->name);
                            
                            objv_mbuf_format(mbuf,"%lld", (* (objv_object_property_longLongValue_t)p->getter->impl)(clazz,object));
                            
                            i ++;
                        }
                        else if(p->type == & objv_type_ulongLong){
                            if( i != 0){
                                objv_mbuf_append(mbuf,",",1);
                            }
                            
                            objv_mbuf_format(mbuf,"\"%s\":",p->name->name);
                            
                            objv_mbuf_format(mbuf,"%llu", (* (objv_object_property_ulongLongValue_t)p->getter->impl)(clazz,object));
                            
                            i ++;
                        }
                        else if(p->type == & objv_type_float){
                            
                            if( i != 0){
                                objv_mbuf_append(mbuf,",",1);
                            }
                            
                            objv_mbuf_format(mbuf,"\"%s\":",p->name->name);
                            
                            objv_mbuf_format(mbuf,"%f", (double) (* (objv_object_property_floatValue_t)p->getter->impl)(clazz,object));
                            
                            i ++;
                        }
                        else if(p->type == & objv_type_double){
                            if( i != 0){
                                objv_mbuf_append(mbuf,",",1);
                            }
                            
                            objv_mbuf_format(mbuf,"\"%s\":",p->name->name);
                            
                            objv_mbuf_format(mbuf,"%lf", (* (objv_object_property_doubleValue_t)p->getter->impl)(clazz,object));
                            
                            i ++;
                        }
                        else if(p->type == & objv_type_boolean){
                            
                            if( i != 0){
                                objv_mbuf_append(mbuf,",",1);
                            }
                        
                            objv_mbuf_format(mbuf,"\"%s\":",p->name->name);
                            
                            if((* (objv_object_property_booleanValue_t)p->getter->impl)(clazz,object)){
                                objv_mbuf_append(mbuf,"true",4);
                            }
                            else{
                                objv_mbuf_append(mbuf,"false",5);
                            }
                            
                            i ++;
                        }
                        else if(p->type == & objv_type_object){
                            
                            if( i != 0){
                                objv_mbuf_append(mbuf,",",1);
                            }
                            
                            objv_mbuf_format(mbuf,"\"%s\":",p->name->name);
                            
                            objv_json_encode_mbuf(zone,(* (objv_object_property_objectValue_t)p->getter->impl)(clazz,object),mbuf);
                            
                            i ++;
                        }
                        
                    }
                    
                    p ++;
                    c --;
                }
                
                clazz = clazz->superClass;
                
            }
             
            objv_mbuf_append(mbuf,"}",1);
        }
    }
}

objv_string_t * objv_json_encode(objv_zone_t * zone,objv_object_t * object){
    
    objv_mbuf_t mbuf;
    objv_string_t * s;
    
    objv_mbuf_init(& mbuf,128);
    
    objv_json_encode_mbuf(zone,object,& mbuf);
    
    s = objv_string_new(zone, objv_mbuf_str(& mbuf));
    
    objv_mbuf_destroy(& mbuf);
    
    return s;
}

#define SPACE_CHAR(c)  ((c) ==' ' || (c) =='\t' || (c) =='\r' || (c) == '\n')

typedef struct _objv_json_t {
    objv_zone_t * zone;
    char * p;
} objv_json_t;

static objv_object_t * objv_json_decode_object(objv_json_t * json);

static void objv_json_skipSpace(objv_json_t * json){
    
    while(json->p && * json->p != 0){
        
        if(SPACE_CHAR( * json->p )){
            
        }
        else{
            break;
        }
        
        json->p ++;
    }
}

static objv_string_t * objv_json_decode_string(objv_json_t * json , char endChar){
    
    objv_string_t * s = NULL;
    objv_mbuf_t mbuf;
    
    objv_mbuf_init(&mbuf,64);
    
    while (* json->p != 0) {
        
        if( * json->p == '\\'){
            if(json->p[1] == 'n'){
                objv_mbuf_append(& mbuf,"\n" ,1);
                json->p ++;
            }
            else if(json->p[1] == 'r'){
                objv_mbuf_append(& mbuf,"\r" ,1);
                json->p ++;
            }
            else if(json->p[1] == 't'){
                objv_mbuf_append(& mbuf,"\t" ,1);
                json->p ++;
            }
            else if(json->p[1] == '\\'){
                objv_mbuf_append(& mbuf,"\\" ,1);
                json->p ++;
            }
            else if(json->p[1] == '"'){
                objv_mbuf_append(& mbuf,"\"" ,1);
                json->p ++;
            }
            else if(json->p[1] == 'u' || json->p[1] == 'U'){
                {
                    
                    int ui = 0;
                    unsigned short unicode = 0;
					
                    sscanf(json->p + 2, "%04x",&ui);
                    
                    unicode = ui;
                    
                    objv_unicode_to_utf8( & unicode , 1, & mbuf);
                    
                    json->p += 5;
                }
            }
        }
        else if( * json->p == endChar){
            json->p ++;
            break;
        }
        else {
            objv_mbuf_append(& mbuf,json->p ,1);
        }
        
        json->p ++;
    }
    
    s =  objv_string_new(json->zone,objv_mbuf_str(& mbuf));
    
    objv_mbuf_destroy(& mbuf);
    
    return s;
}

static objv_string_t * objv_json_decode_key(objv_json_t * json){
    
    objv_string_t * s = NULL;
    objv_mbuf_t mbuf;
    
    if( * json->p == '\'' || * json->p == '"'){
        return objv_json_decode_string(json,* json->p);
    }
    else{
        
        objv_mbuf_init(&mbuf,64);
        
        while (* json->p != 0) {
            
            if(SPACE_CHAR( * json->p ) || * json->p == ':'
               || * json->p == '}' || * json->p == ']' || * json->p == ','){
                break;
            }
            else{
                objv_mbuf_append(& mbuf,json->p , 1);
            }
            
            json->p ++;
        }
        
        if(mbuf.length >0){
            s =  objv_string_new(json->zone,objv_mbuf_str(& mbuf));
        }
        
        objv_mbuf_destroy(& mbuf);
    }
    
    return s;
}

static objv_dictionary_t * objv_json_decode_dictionary(objv_json_t * json){
    
    objv_dictionary_t * object = objv_dictionary_new(json->zone,4);
    
    objv_string_t * key;
    objv_object_t * v;
    
    while((key = objv_json_decode_key(json))){
        
        objv_json_skipSpace(json);
        
        if( * json->p == ':'){
            json->p ++;
            
            v = objv_json_decode_object(json);
            
            objv_dictionary_setValue(object,(objv_object_t *)key,v);
            
            objv_json_skipSpace(json);
            
            if(* json->p == ','){
                json->p ++;
            }
            else{
                
                while (* json->p != 0) {
                    if(* json->p == '}'){
                        break;
                    }
                    json->p ++;
                }
                
                if( * json->p == '}'){
                    json->p ++;
                    break;
                }
                else{
                    break;
                }
            }
        }
        else if( * json->p == '}'){
            json->p ++;
            break;
        }
        else{
            break;
        }
    }
    
    return object;
}

static objv_array_t * objv_json_decode_array(objv_json_t * json){
    
    objv_array_t * array = objv_array_new(json->zone,4);
    
    objv_object_t * v;
    
    while (1) {
        
        objv_json_skipSpace(json);
        
        if( * json->p == 0){
            break;
        }
        
        if(* json->p == ']'){
            json->p ++;
            break;
        }
        
        v = objv_json_decode_object(json);
        
        objv_array_add(array,v);
        
    }
    
    return array;
    
}

static objv_value_t * objv_json_decode_value(objv_json_t * json){
    
    objv_value_t * v = NULL;
    
    objv_mbuf_t mbuf;
    
    double dv;
    
    objv_mbuf_init(& mbuf,64);
    
    while(json->p && * json->p != 0){
        
        if(SPACE_CHAR( * json->p ) || * json->p == ':'
           || * json->p == ',' || * json->p == '}' || * json->p == ']'){
            break;
        }
        else{
            objv_mbuf_append(& mbuf,json->p,1);
        }
        json->p ++;
    }
    
    if(strcmp(objv_mbuf_str(& mbuf),"null") ==0){
        v = (objv_value_t *) objv_object_retain((objv_object_t *)objv_value_alloc_nullValue(json->zone));
    }
    else if(strcmp(objv_mbuf_str(& mbuf),"false") ==0){
        v = (objv_value_t *) objv_object_retain((objv_object_t *)objv_value_alloc_booleanValue(json->zone,objv_false));
    }
    else if(strcmp(objv_mbuf_str(& mbuf),"true") ==0){
        v = (objv_value_t *) objv_object_retain((objv_object_t *)objv_value_alloc_booleanValue(json->zone,objv_true));
    }
    else {
        dv = atof(objv_mbuf_str(& mbuf));
        if((double)(long long) dv == dv){
            v = (objv_value_t *) objv_object_retain((objv_object_t *)objv_value_alloc_longLongValue(json->zone,(long long) dv));
        }
        else{
            v = (objv_value_t *) objv_object_retain((objv_object_t *)objv_value_alloc_doubleValue(json->zone,dv));
        }
    }
    
    objv_mbuf_destroy(& mbuf);
    
    return v;
}



static objv_object_t * objv_json_decode_object(objv_json_t * json){
    
    objv_json_skipSpace(json);
    
    if(json->p && * json->p == '\''){
        json->p ++;
        return (objv_object_t *) objv_json_decode_string(json,'\'');
    }
    else if(json->p &&  * json->p == '"'){
        json->p ++;
        return (objv_object_t *) objv_json_decode_string(json,'"');
    }
    else if(json->p &&  * json->p == '{'){
        json->p ++;
        return (objv_object_t *) objv_json_decode_dictionary(json);
    }
    else if(json->p &&  * json->p == '['){
        json->p ++;
        return (objv_object_t *) objv_json_decode_array(json);
    }
    else{
        return (objv_object_t *) objv_json_decode_value(json);
    }
}

objv_object_t * objv_json_decode(objv_zone_t * zone,const char * text){
    objv_json_t json = {zone,(char *) text};
    return objv_json_decode_object( & json);;
}
