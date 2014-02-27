//
//  objv_clcontext.c
//  objv
//
//  Created by zhang hailong on 14-2-10.
//  Copyright (c) 2014年 hailong.org. All rights reserved.
//

#define READONLY

#include "objv_os.h"
#include "objv.h"
#include "objv_cloud.h"
#include "objv_clcontext.h"
#include "objv_value.h"
#include "objv_autorelease.h"

OBJV_KEY_IMP(CLServiceContext)

static void CLServiceContextMethodDealloc(objv_class_t * clazz,objv_object_t * object){

    CLServiceContext * ctx = (CLServiceContext *) object;
    
    objv_object_release((objv_object_t *) ctx->container);
    
    if(clazz->superClass){
        objv_object_dealloc(clazz->superClass, object);
    }
}

static void CLServiceContextMethodHandleTask(objv_class_t * clazz,CLServiceContext * ctx,objv_class_t * taskType,CLTask * task){
    
    if(ctx->container){
        
        CLServiceContainerHandleTask(ctx->container, (CLContext *) ctx, taskType, task);
        
    }
    
}

static void CLServiceContextMethodSendTask(objv_class_t * clazz,CLServiceContext * ctx,objv_class_t * taskType,CLTask * task){
    CLContextHandleTask((CLContext *) ctx, taskType, task);
}

OBJV_CLASS_METHOD_IMP_BEGIN(CLServiceContext)

OBJV_CLASS_METHOD_IMP(dealloc, "v()", CLServiceContextMethodDealloc)

OBJV_CLASS_METHOD_IMP(handleTask, "v(*,@)", CLServiceContextMethodHandleTask)

OBJV_CLASS_METHOD_IMP(sendTask, "v(*,@)", CLServiceContextMethodSendTask)

OBJV_CLASS_METHOD_IMP_END(CLServiceContext)

OBJV_CLASS_IMP_M(CLServiceContext, OBJV_CLASS(CLContext), CLServiceContext)

void CLServiceContextSetContainer(CLServiceContext * ctx,CLServiceContainer * container){
    if(ctx && ctx->container != container){
        objv_object_retain((objv_object_t *) container);
        objv_object_release((objv_object_t *) ctx->container);
        ctx->container = container;
    }
}


OBJV_KEY_IMP(CLChannelContext);


static void CLChannelContextMethodDealloc(objv_class_t * clazz,objv_object_t * object){
    
    CLChannelContext * ctx = (CLChannelContext *) object;

    objv_mutex_lock(& ctx->channels_mutex);
    
    objv_object_release((objv_object_t *) ctx->channels );
    
    ctx->channels = NULL;
    
    objv_mutex_unlock(& ctx->channels_mutex);
    
    objv_object_release((objv_object_t *) ctx->queue);
    
    if(clazz->superClass){
        objv_object_dealloc(clazz->superClass, object);
    }
}

static objv_object_t * CLChannelContextMethodInit(objv_class_t * clazz,objv_object_t * object,va_list ap){
    
    if(clazz->superClass){
        object = objv_object_initv(clazz->superClass, object, ap);
    }
    
    if(object){
        
        objv_zone_t * zone = object->zone;
        
        CLChannelContext * ctx = (CLChannelContext *) object;
        
        objv_mutex_init(& ctx->channels_mutex);
        
        objv_mutex_lock(& ctx->channels_mutex);
        
        ctx->channels = objv_array_alloc(zone, 4);
        
        objv_mutex_unlock(& ctx->channels_mutex);
        
    }
    
    return object;
}


static void  CLChannelContextMethodSendTaskFun (objv_class_t * clazz,CLChannelContext * ctx,objv_class_t * taskType,CLTask * task){
    
    if(task->source == (CLContext *) ctx){
        CLContextHandleTask((CLContext *) ctx, taskType, task);
    }
    else{
        
        objv_mutex_lock(& ctx->channels_mutex);
        
        if( ctx->channels && ctx->channels->length > 0){
            
            if(ctx->channel_index >= ctx->channels->length){
                ctx->channel_index = 0;
            }
            
            CLChannel * channel = (CLChannel *) objv_array_objectAt(ctx->channels, ctx->channel_index ++);
            
            CLChannelPostTask(channel->base.isa, channel, task, taskType);
            
        }
        
        objv_mutex_unlock(& ctx->channels_mutex);
    }
}

static void CLChannelContextMethodSetConfig(objv_class_t * clazz,CLChannelContext * ctx,objv_object_t * config){
 
    if(clazz->superClass){
        CLContextSetConfig(clazz->superClass, (CLContext *) ctx, config);
    }
    
    objv_zone_t * zone = ctx->base.base.base.zone;
    
    if(ctx->queue == NULL){
        ctx->queue = objv_dispatch_queue_alloc(zone, "CLChannelContext", objv_object_uintValueForKey(config, (objv_object_t *)objv_string_new_nocopy(zone, "maxThreadCount"), 20));
    }
    
}


OBJV_CLASS_METHOD_IMP_BEGIN(CLChannelContext)

OBJV_CLASS_METHOD_IMP(dealloc, "v()", CLChannelContextMethodDealloc)

OBJV_CLASS_METHOD_IMP(init, "@(*)", CLChannelContextMethodInit)

OBJV_CLASS_METHOD_IMP(sendTask, "v(*,@)", CLChannelContextMethodSendTaskFun)

OBJV_CLASS_METHOD_IMP(setConfig, "v(@)", CLChannelContextMethodSetConfig)

OBJV_CLASS_METHOD_IMP_END(CLChannelContext)

OBJV_CLASS_IMP_M(CLChannelContext,OBJV_CLASS(CLContext),CLChannelContext)


typedef struct _CLChannelContextTask {
    objv_dispatch_task_t base;
    CLChannel * channel;
    CLChannelContext * ctx;
} CLChannelContextTask;

OBJV_KEY_DEC(CLChannelContextTask)

OBJV_CLASS_DEC(CLChannelContextTask)

OBJV_KEY_IMP(CLChannelContextTask)

static void CLChannelContextTaskRun(objv_class_t * clazz, CLChannelContextTask * task){
    
    
    OBJVChannelStatus status = OBJVChannelStatusOK;
    
    objv_timeinval_t delay = 0.02;
    
    if((status = CLChannelConnect(task->channel->base.isa, task->channel, 0.02)) == OBJVChannelStatusOK){
        
        
        {
            CLTask * t = NULL;
            objv_class_t * tType = NULL;
            
            if((status = CLChannelReadTask(task->channel->base.isa,task->channel, & t, & tType, 0.02)) == OBJVChannelStatusOK){
                
                if(t && tType){
                    
                    CLContextHandleTask((CLContext *) task->ctx, tType, t);
                    
                }
                
            }
            
        }
        
        if(status != OBJVChannelStatusError){
            status = CLChannelTick(task->channel->base.isa,task->channel, 0.02);
        }
        
    }
    
    if(status == OBJVChannelStatusError){
        CLChannelContextRemoveChannel(task->ctx, task->channel);
    }
    else {
        task->base.delay = delay;
        objv_dispatch_queue_addTask(task->ctx->queue, (objv_dispatch_task_t *) task);
    }
}

static void CLChannelContextTaskDealloc(objv_class_t * clazz, objv_object_t * object){
    
    CLChannelContextTask * task =(CLChannelContextTask *) object;
    
    objv_object_release((objv_object_t *)task->ctx);
    objv_object_release((objv_object_t *)task->channel);
    
    if(clazz->superClass){
        objv_object_dealloc(clazz->superClass, object);
    }
}

static objv_object_t * CLChannelContextTaskInit(objv_class_t * clazz, objv_object_t * object,va_list ap){
    
    if(clazz->superClass){
        object = objv_object_initv(clazz->superClass, object, ap);
    }
    
    if(object){
        
        CLChannelContextTask * task =(CLChannelContextTask *) object;
        
        task->ctx = (CLChannelContext *) objv_object_retain(va_arg(ap, objv_object_t *));
        task->channel = (CLChannel *) objv_object_retain(va_arg(ap, objv_object_t *));
        
    }
    
    return object;
}

OBJV_CLASS_METHOD_IMP_BEGIN(CLChannelContextTask)

OBJV_CLASS_METHOD_IMP(run, "v()", CLChannelContextTaskRun)
OBJV_CLASS_METHOD_IMP(dealloc, "v()", CLChannelContextTaskDealloc)
OBJV_CLASS_METHOD_IMP(init, "@(*)", CLChannelContextTaskInit)

OBJV_CLASS_METHOD_IMP_END(CLChannelContextTask)

OBJV_CLASS_IMP_M(CLChannelContextTask, OBJV_CLASS(DispatchTask), CLChannelContextTask)

void CLChannelContextAddChannel(CLChannelContext * ctx,CLChannel * channel){
    if(ctx && channel && ctx->queue){
        
        objv_zone_t * zone = ctx->base.base.base.zone;
        
        CLChannelContextTask * task = (CLChannelContextTask *) objv_object_alloc(zone, OBJV_CLASS(CLChannelContextTask),ctx,channel,NULL);
        
        objv_dispatch_queue_addTask(ctx->queue, (objv_dispatch_task_t *) task);
        
        objv_object_release((objv_object_t *) task);
    }
}

void CLChannelContextRemoveChannel(CLChannelContext * ctx,CLChannel * channel){
    if(ctx && channel ){
        
        objv_mutex_lock(& ctx->channels_mutex);
        
        objv_array_remove(ctx->channels,(objv_object_t *) channel);
        
        objv_mutex_unlock(& ctx->channels_mutex);

    }
}
