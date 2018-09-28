//
//  kk-canvas.cc
//  KKCanvas
//
//  Created by zhanghailong on 2018/9/28.
//  Copyright © 2018年 kkmofang.cn. All rights reserved.
//


#include "kk-config.h"
#include "kk-canvas.h"
#include "require_js.h"
#include "CGContext.h"

#define Kernel 1.0

namespace kk {
    
    IMP_SCRIPT_CLASS_BEGIN_NOALLOC(nullptr, Canvas, Canvas)
    
    static kk::script::Property propertys[] = {
        IMP_SCRIPT_PROPERTY(Canvas,width,Width)
        IMP_SCRIPT_PROPERTY(Canvas,height,Height)
    };
    
    kk::script::SetProperty(ctx, -1, propertys, sizeof(propertys) / sizeof(kk::script::Property));
    
    static kk::script::Method methods[] = {
        IMP_SCRIPT_METHOD(Canvas,getContext)
    };
    
    kk::script::SetMethod(ctx, -1, methods, sizeof(methods) / sizeof(kk::script::Method));
    
  
    IMP_SCRIPT_CLASS_END
    
    static void Canvas_getString(kk::CString basePath,kk::CString path,kk::String & out) {
        
        kk::String p;
        
        if(basePath) {
            
            p.append(basePath);
            
            if(!kk::CStringHasSuffix(basePath, "/")) {
                p.append("/");
            }
            
        }
        
        p.append(path);
        

        FILE * fd = fopen(p.c_str(), "r");
        
        if(fd) {
            
            char data[20480];
            size_t n;
            
            while((n = fread(data, 1, sizeof(data), fd)) > 0) {
                out.append(data,0,n);
            }
            
            fclose(fd);
        } else{
            kk::Log("Not Open %s",p.c_str());
        }
        
    }
    
    static duk_ret_t Canvas_getString(duk_context * ctx) {
        
        kk::CString basePath = nullptr;
        
        duk_get_global_string(ctx, "__basePath");
        
        if(duk_is_string(ctx, -1)) {
            basePath = duk_to_string(ctx, -1);
        }
        
        duk_pop(ctx);
        
        if(basePath) {
            
            int top = duk_get_top(ctx);
            
            kk::CString path = nullptr;
            
            if(top >0 && duk_is_string(ctx, -top)) {
                path = duk_to_string(ctx, -top);
            }
            
            if(path) {
                kk::String v;
                Canvas_getString(basePath,path,v);
                duk_push_string(ctx, v.c_str());
                return 1;
            }
            
        }
        
        return 0;
    }
    
    static duk_ret_t Canvas_compile(duk_context * ctx) {
        
        kk::CString basePath = nullptr;
        
        duk_get_global_string(ctx, "__basePath");
        
        if(duk_is_string(ctx, -1)) {
            basePath = duk_to_string(ctx, -1);
        }
        
        duk_pop(ctx);
        
        if(basePath) {
            
            kk::CString path = nullptr;
            kk::CString prefix = nullptr;
            kk::CString suffix = nullptr;
            
            int top = duk_get_top(ctx);
            
            if(top > 0  && duk_is_string(ctx, - top)) {
                path = duk_to_string(ctx, -top);
            }
            
            if(top > 1  && duk_is_string(ctx, - top + 1)) {
                prefix = duk_to_string(ctx, -top + 1);
            }
            
            if(top > 2  && duk_is_string(ctx, - top + 2)) {
                suffix = duk_to_string(ctx, -top + 2);
            }
            
            kk::String code;
            
            if(prefix) {
                code.append(prefix);
            }
            
            Canvas_getString(basePath,path,code);
            
            if(suffix) {
                code.append(suffix);
            }
            
            duk_push_string(ctx, path);
            duk_compile_string_filename(ctx, 0, code.c_str());
            
            return 1;
        }
        
        return 0;
    }
    
    static void CanvasOpenlibs(duk_context * ctx,kk::DispatchQueue * queue,
                               evdns_base * dns,
                               kk::CString basePath);
    
    static void Canvas_CreateContext (duk_context * ctx, kk::DispatchQueue * queue, duk_context * newContext) {
        
        kk::CString basePath = nullptr;
        
        duk_get_global_string(ctx, "__basePath");
        
        if(duk_is_string(ctx, -1)) {
            basePath = duk_to_string(ctx, -1);
        }
        
        duk_pop(ctx);
        
        CanvasOpenlibs(newContext,queue,ev_dns(ctx),basePath);
        
    }
    
    static void CanvasOpenlibs(duk_context * ctx,kk::DispatchQueue * queue,
                               evdns_base * dns,
                               kk::CString basePath) {
        
        {
            duk_push_global_object(ctx);
            duk_push_string(ctx,"__basePath");
            duk_push_string(ctx, basePath);
            duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE|DUK_DEFPROP_CLEAR_WRITABLE);
            duk_pop(ctx);
        }
        
        {
            
            duk_push_global_object(ctx);
            
            duk_push_string(ctx, "kk");
            duk_push_object(ctx);
            
            duk_push_string(ctx, "platform");
            duk_push_string(ctx, "kk");
            duk_put_prop(ctx, -3);
            
            duk_push_string(ctx, "kernel");
            duk_push_number(ctx, Kernel);
            duk_put_prop(ctx, -3);
            
            duk_push_string(ctx, "getString");
            duk_push_c_function(ctx, Canvas_getString, 1);
            duk_put_prop(ctx, -3);
            
            duk_push_string(ctx, "compile");
            duk_push_c_function(ctx, Canvas_compile, 3);
            duk_put_prop(ctx, -3);
            
            duk_put_prop(ctx, -3);
            
            
            duk_pop(ctx);
        }
        
        {
            duk_eval_lstring_noresult(ctx, (char *) require_js, sizeof(require_js));
            kk::Crypto_openlibs(ctx);
        }
        
    }
    
    static duk_ret_t Canvas_draw_func(duk_context * ctx) {
        
        Canvas * object = nullptr;
        
        CanvasDrawFunc func = nullptr;
        
        duk_push_current_function(ctx);
        
        duk_get_prop_string(ctx, -1, "__func");
        
        if(duk_is_pointer(ctx, -1)) {
            func = (CanvasDrawFunc) duk_to_pointer(ctx, -1);
        }
        
        duk_pop_2(ctx);
        
        duk_push_this(ctx);
        
        duk_get_prop_string(ctx, -1, "__object");
        
        if(duk_is_pointer(ctx, -1)) {
            object = (Canvas *) duk_to_pointer(ctx, -1);
        }
        
        duk_pop_2(ctx);
        
        if(object && func) {
            kk::Strong v = object->popCGContext();
            if(v.get() != nullptr) {
                (*func)(object,v.get());
            }
        }
        
        return 0;
    }
    
    static duk_ret_t Canvas_emit_func(duk_context * ctx) {
        
        Canvas * object = nullptr;
        
        CanvasEmitFunc func = nullptr;
        
        duk_push_current_function(ctx);
        
        duk_get_prop_string(ctx, -1, "__func");
        
        if(duk_is_pointer(ctx, -1)) {
            func = (CanvasEmitFunc) duk_to_pointer(ctx, -1);
        }
        
        duk_pop_2(ctx);
        
        duk_push_this(ctx);
        
        duk_get_prop_string(ctx, -1, "__object");
        
        if(duk_is_pointer(ctx, -1)) {
            object = (Canvas *) duk_to_pointer(ctx, -1);
        }
        
        duk_pop_2(ctx);
        
        if(object && func) {
            (*func)(object);
        }
        
        return 0;
    }
    
    Canvas::Canvas(kk::DispatchQueue * queue,
           evdns_base * dns,
           kk::CString basePath,
           const CanvasCallback * cb,void * userdata):
        _queue(queue),_basePath(basePath),_userdata(userdata){
        
        _jsContext = new kk::script::Context();
        
        duk_context * ctx = dukContext();
        
        kk::ev_openlibs(ctx, queue->base(), dns);
        kk::wk_openlibs(ctx, queue, Canvas_CreateContext);
        
        kk::script::SetPrototype(ctx, &Canvas::ScriptClass);
            
        CanvasOpenlibs(ctx,queue,dns,basePath);
        
        {
            duk_push_object(ctx);
            
            duk_push_string(ctx, "__object");
            duk_push_pointer(ctx, this);
            duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE|DUK_DEFPROP_CLEAR_WRITABLE);
            
            duk_push_string(ctx, "draw");
            duk_push_c_function(ctx, Canvas_draw_func, 0);
                duk_push_string(ctx, "__func");
                duk_push_pointer(ctx, (void *) cb->draw);
                duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE|DUK_DEFPROP_CLEAR_WRITABLE);
            duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE|DUK_DEFPROP_CLEAR_WRITABLE);
            
            duk_push_string(ctx, "emit");
            duk_push_c_function(ctx, Canvas_emit_func, 0);
                duk_push_string(ctx, "__func");
                duk_push_pointer(ctx, (void *) cb->emit);
                duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE|DUK_DEFPROP_CLEAR_WRITABLE);
            duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE | DUK_DEFPROP_CLEAR_ENUMERABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE|DUK_DEFPROP_CLEAR_WRITABLE);
            
            if(kk::script::GetPrototype(ctx, &Canvas::ScriptClass)) {
                duk_set_prototype(ctx, -2);
            }
            
            duk_put_global_string(ctx, "canvas");
        }
        
        
    }
    
    kk::script::Context * Canvas::jsContext() {
        return _jsContext.as<kk::script::Context>();
    }
    
    duk_context * Canvas::dukContext() {
        kk::script::Context * v = jsContext();
        if(v != nullptr) {
            return v->jsContext();
        }
        return nullptr;
    }
    
    static void Canvas_set(DispatchQueue * queue,BK_DEF_ARG) {
        
        BK_GET(key, char)
        BK_GET(value, char)
        BK_GET_STRONG(canvas)
        
        Canvas * v = canvas.as<Canvas>();
        
        if(v == nullptr) {
            return;
        }
        
        duk_context * ctx = v->dukContext();
        
        if(ctx) {
            
            duk_get_global_string(ctx, "canvas");
            
            duk_push_string(ctx, key);
            
            if(value ==nullptr) {
                duk_del_prop(ctx, -2);
            } else {
                duk_push_string(ctx, value);
                duk_put_prop(ctx, -3);
            }
            
            duk_get_prop_string(ctx, -1, "on");
            
            if(duk_is_function(ctx, -1)) {
                
                duk_push_string(ctx, key);
                
                if(value == nullptr) {
                    duk_push_undefined(ctx);
                } else {
                    duk_push_string(ctx, value);
                }
                
                if(duk_pcall(ctx, 2) != DUK_EXEC_SUCCESS) {
                    kk::script::Error(ctx, -1);
                }
            }
            
            duk_pop_2(ctx);
            
        }
    }
    
    void Canvas::set(kk::CString key, kk::CString value) {
        
        if(key == nullptr) {
            return;
        }
        
        BK_CTX
        
        BK_CSTRING(key, key)
        if(value != nullptr) {
            BK_CSTRING(value, value)
        }
        BK_WEAK(canvas, this)
        
        _queue.as<DispatchQueue>()->async(Canvas_set, BK_ARG);
    }
    
    static void Canvas_exec(DispatchQueue * queue,BK_DEF_ARG) {
        
        BK_GET(code, char)
        BK_GET(filename, char)
        BK_GET_STRONG(canvas)
        
        Canvas * v = canvas.as<Canvas>();
        
        if(v == nullptr) {
            return;
        }
        
        duk_context * ctx = v->dukContext();
        
        if(ctx) {
            
            duk_push_string(ctx, filename);
            duk_compile_string_filename(ctx, 0, code);
            
            if(duk_is_function(ctx, -1)) {
                
                if(duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS) {
                    kk::script::Error(ctx, -1);
                }
            }
            
            duk_pop(ctx);
            
        }
    }
    
    void Canvas::exec(kk::CString code,kk::CString filename) {
        
        if(code == nullptr || filename == nullptr) {
            return;
        }
        
        BK_CTX
        
        BK_CSTRING(code, code)
        BK_CSTRING(filename, filename)
        BK_WEAK(canvas, this)
        
        _queue.as<DispatchQueue>()->async(Canvas_exec, BK_ARG);
    }
    
    void Canvas::exec(kk::CString path) {
        kk::String v;
        Canvas_getString(_basePath.c_str(), path, v);
        exec(v.c_str(), path);
    }
    
    void * Canvas::userdata() {
        return _userdata;
    }
    
    void Canvas::setUserdata(void *userdata) {
        _userdata = userdata;
    }
    
    static void Canvas_onresize(DispatchQueue * queue,BK_DEF_ARG) {
        
        BK_GET_STRONG(canvas)
        
        Canvas * v = canvas.as<Canvas>();
        
        if(v == nullptr) {
            return;
        }
        
        duk_context * ctx = v->dukContext();
        
        if(ctx) {
            
            duk_get_global_string(ctx, "canvas");
            
            if(duk_is_object(ctx, -1)) {
                
                duk_get_prop_string(ctx, -1, "onresize");
                
                if(duk_is_function(ctx, -1)) {
                    
                    if(duk_pcall(ctx, 0) != DUK_EXEC_SUCCESS) {
                        kk::script::Error(ctx, -1);
                    }
                }
                
            }
            
            duk_pop_2(ctx);
            
        }
    }
    
    void Canvas::setSize(Uint width,Uint height) {
        
        if(_width != width || _height != height) {
            
            _width = width;
            _height = height;
            
            BK_CTX

            BK_WEAK(canvas, this)
            
            _queue.as<DispatchQueue>()->async(Canvas_onresize, BK_ARG);
        }
    }
    
    kk::Strong Canvas::popCGContext() {
        kk::Strong v = _CGContext.get();
        _CGContext = (kk::Object *) nullptr;
        return v;
    }
    
    duk_ret_t Canvas::duk_width(duk_context *ctx) {
        duk_push_uint(ctx, _width);
        return 1;
    }
    
    duk_ret_t Canvas::duk_height(duk_context *ctx) {
        duk_push_uint(ctx, _width);
        return 1;
    }
    
    duk_ret_t Canvas::duk_setWidth(duk_context *ctx) {
        
        int top = duk_get_top(ctx);
        
        if(top > 0) {
            Int v = kk::script::toInt(ctx, -top);
            if(v >=0 ) {
                _width = v;
            }
        }
        return 0;
    }
    
    duk_ret_t Canvas::duk_setHeight(duk_context *ctx) {
        
        int top = duk_get_top(ctx);
        
        if(top > 0) {
            Int v = kk::script::toInt(ctx, -top);
            if(v >=0 ) {
                _height = v;
            }
        }
        return 0;
    }
    
    duk_ret_t Canvas::duk_getContext(duk_context *ctx) {
        
        kk::String name;
        
        int top = duk_get_top(ctx);
        
        if(top > 0) {
            name = kk::script::toString(ctx, -top);
        }
        
        if(name == "2d") {
            
            kk::CG::Context * v = _CGContext.as<kk::CG::Context>();
            
            if(v != nullptr && (v->width() != _width || v->height() != _height)) {
                v = nullptr;
                _CGContext = (kk::Object *) nullptr;
            }
            
            if(v == nullptr && _width > 0 && _height > 0) {
                v = new kk::CG::OSContext(_width,_height);
                _CGContext = v;
            }
            
            kk::script::PushObject(ctx, v);
            
            return 1;
        }
        
        return 0;
    }
    
}
