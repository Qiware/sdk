/******************************************************************************
 ** Copyright(C) 2014-2024 Qiware technology Co., Ltd
 **
 ** 文件名: xml_print.c
 ** 版本号: 1.0
 ** 描  述: XML格式的打印
 **        将XML树打印成XML格式的文件、字符串等.
 ** 作  者: # Qifeng.zou # 2013.02.18 #
 ******************************************************************************/
#include "log.h"
#include "comm.h"
#include "xml_tree.h"
#include "xml_comm.h"

/******************************************************************************
 **                                                                         **
 **                             打印成有层次结构                            **
 **                                (文件中)                                 **
 ******************************************************************************/
/* 打印节点名(注: XML有层次格式) */
#define xml_fprint_name(fp, node, depth) \
{ \
    while (depth > 1) \
    {   \
        fprintf(fp, "\t"); \
        depth--; \
    } \
    fprintf(fp, "<%s", node->name.str); \
}

/* 打印属性节点(注: XML有层次格式) */
#define xml_fprint_attr(fp, node) \
{ \
    while (NULL != node->temp) \
    { \
        if (xml_is_attr(node->temp)) \
        { \
            fprintf(fp, " %s=\"%s\"", node->temp->name.str, node->temp->value.str); \
            node->temp = node->temp->next; \
            continue; \
        } \
        break; \
    } \
}

/* 打印节点值(注: XML有层次格式) */
#define xml_fprint_value(fp, node) \
{ \
    if (xml_has_value(node)) \
    { \
        if (xml_has_child(node))  /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            fprintf(fp, ">%s\n", node->value.str); \
        } \
        else \
        { \
            fprintf(fp, ">%s</%s>\n", node->value.str, node->name.str); \
        } \
    } \
    else \
    { \
        if (NULL != node->temp)   /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            fprintf(fp, ">\n"); \
        } \
        else \
        { \
            fprintf(fp, "/>\n"); \
        } \
    } \
}

/******************************************************************************
 **函数名称: xml_fprint_next
 **功    能: 选择下一个处理的节点(注: XML有层次格式)
 **输入参数:
 **     root: 根节点
 **     stack: 栈
 **     fp: 文件指针
 **输出参数:
 **返    回: 0: 成功  !0: 失败
 **实现描述: 
 **注意事项: 
 **     1. 刚开始时，栈的深度必须为1.
 **     2. 此时temp用来记录正在处理孩子节点
 **     3. 不打印root的兄弟节点
 **作    者: # Qifeng.zou # 2013.02.27 #
 ******************************************************************************/
static xml_node_t *xml_fprint_next(
        xml_tree_t *xml, Stack_t *stack, xml_node_t *node, FILE *fp)
{
    int depth, level;
    xml_node_t *top = NULL, *child = NULL;

    if (NULL != node->temp)      /* 首先: 处理孩子节点: 选出下一个孩子节点 */
    {
        child = node->temp;
        node->temp = child->next;
        node = child;
        return node;
    }
    else                        /* 再次: 处理其兄弟节点: 选出下一个兄弟节点 */
    {
        /* 1. 弹出已经处理完成的节点 */
        top = stack_pop(stack);
        if (xml_has_child(top))
        {
            depth = stack_depth(stack);
            level = depth - 1;
            while (level > 1)
            {
                fprintf(fp, "\t");
                level--;
            }
            fprintf(fp, "</%s>\n", top->name.str);
        }

        if (stack_empty(stack))
        {
            //log_error(xml->log, "Compelte fprint!");
            return NULL;
        }
        
        /* 2. 处理其下一个兄弟节点 */
        node = top->next;
        while (NULL == node)     /* 所有兄弟节点已经处理完成，说明父亲节点也处理完成 */
        {
            /* 3. 父亲节点出栈 */
            top = stack_pop(stack);
            if (NULL == top)
            {
                //log_error(xml->log, "Stack pop failed!");
                return NULL;
            }
        
            /* 4. 打印父亲节点结束标志 */
            if (xml_has_child(top))
            {
                depth = stack_depth(stack);
                level = depth + 1;
                while (level > 1)
                {
                    fprintf(fp, "\t");
                    level--;
                }
                fprintf(fp, "</%s>\n", top->name.str);
            }
            
            if (stack_empty(stack))
            {
                return NULL;    /* 处理完成 */
            }

            /* 5. 选择父亲的兄弟节点 */
            node = top->next;
        }
    }    
    
    return node;
}

/******************************************************************************
 **函数名称: xml_fprint_tree
 **功    能: 将栈中XML节点的相关信息打印至文件(注: XML有层次格式)
 **输入参数:
 **     root: 根节点
 **     stack: 栈
 **     fp: 文件指针
 **输出参数:
 **返    回: 0: 成功  !0: 失败
 **实现描述: 
 **注意事项: 
 **     1. 刚开始时，栈的深度必须为1.
 **     2. 此时temp用来记录正在处理孩子节点
 **     3. 不打印root的兄弟节点
 **作    者: # Qifeng.zou # 2013.02.27 #
 ******************************************************************************/
int xml_fprint_tree(xml_tree_t *xml, xml_node_t *root, Stack_t *stack, FILE *fp)
{
    int depth;
    xml_node_t *node = root;

    depth = stack_depth(stack);
    if (0 != depth)
    {
        //log_error(xml->log, "Stack depth must empty. depth:[%d]", depth);
        return XML_ERR_STACK;
    }

    do
    {
        /* 1. 将要处理的节点压栈 */
        node->temp = node->child;
        if (stack_push(stack, node))
        {
            //log_error(xml->log, "Stack push failed!");
            return XML_ERR_STACK;
        }
        
        /* 2. 打印节点名 */
        depth = stack_depth(stack);
        
        xml_fprint_name(fp, node, depth);
        
        /* 3. 打印属性节点 */
        if (xml_has_attr(node))
        {
            xml_fprint_attr(fp, node);
        }
        
        /* 4. 打印节点值 */
        xml_fprint_value(fp, node);
        
        /* 5. 选择下一个处理的节点: 从父亲节点、兄弟节点、孩子节点中 */
        node = xml_fprint_next(xml, stack, node, fp);
    } while (NULL != node);

    if (!stack_empty(stack))
    {
        return XML_ERR_STACK;
    }
    return XML_OK;
}

/******************************************************************************
 **                                                                          **
 **                              打印成无层次结构                            **
 **                                 (内存中)                                 **
 ******************************************************************************/
/* 组包节点名的长度(注: XML无层次格式) */
#define xml_pack_name_length(node, length) \
{ \
    /*fprintf(fp, "<%s", node->name);*/ \
    length += (1 + node->name.len); \
}

/* 组包属性节点的长度(注: XML无层次格式) */
#define xml_pack_attr_length(node, length) \
{ \
    while (NULL != node->temp) \
    { \
        if (xml_is_attr(node->temp)) \
        { \
            /* sprintf(sp->ptr, " %s=\"%s\"", node->temp->name, node->temp->value); */ \
            /* sp->ptr += strlen(sp->ptr); */ \
            length += node->temp->name.len + node->temp->value.len + 4; \
            node->temp = node->temp->next; \
            continue; \
        } \
        break; \
    } \
}

#if defined(__XML_PACK_CMARK__)
/* 组包节点值的长度(注: XML无层次格式) */
#define xml_pack_value_length(node, length) \
{ \
    if (xml_has_value(node)) \
    { \
        if (xml_has_child(node))  /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            /* sprintf(sp->ptr, ">%s", node->value); */ \
            length += node->value.len+1; \
        } \
        else \
        { \
            /* sprintf(sp->ptr, ">%s</%s>", node->value, node->name); */ \
            length += node->value.len + node->name.len + 4; \
        } \
    } \
    else \
    { \
        if (NULL != node->temp)   /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            /* sprintf(sp->ptr, ">"); */ \
            length++; \
        } \
        else \
        { \
            /* sprintf(sp->ptr, "></%s>", node->name); */ \
            length += node->name.len + 4; \
        } \
    } \
}
#else /*__XML_PACK_CMARK__*/
/* 组包节点值的长度(注: XML无层次格式) */
#define xml_pack_value_length(node, length) \
{ \
    if (xml_has_value(node)) \
    { \
        if (xml_has_child(node))  /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            /* sprintf(sp->ptr, ">%s", node->value); */ \
            length += node->value.len+1; \
        } \
        else \
        { \
            /* sprintf(sp->ptr, ">%s</%s>", node->value, node->name); */ \
            length += node->value.len + node->name.len + 4; \
        } \
    } \
    else \
    { \
        if (NULL != node->temp)   /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            /* sprintf(sp->ptr, ">"); */ \
            length++; \
        } \
        else \
        { \
            /* sprintf(sp->ptr, "/>"); */ \
            length += 2; \
        } \
    } \
}
#endif /*__XML_PACK_CMARK__*/

/******************************************************************************
 **函数名称: xml_pack_next_length
 **功    能: 选择下一个处理的节点, 并计算组包结束节点报文的长度(注: XML无层次格式)
 **输入参数:
 **     root: 根节点
 **     stack: 栈
 **     length: 长度统计
 **输出参数:
 **返    回: 0: 成功  !0: 失败
 **实现描述: 
 **注意事项: 
 **     1. 刚开始时，栈的深度必须为1.
 **     2. 此时temp用来记录正在处理孩子节点
 **     3. 不打印root的兄弟节点
 **作    者: # Qifeng.zou # 2013.06.11 #
 ******************************************************************************/
static xml_node_t *xml_pack_next_length(
        xml_tree_t *xml, Stack_t *stack, xml_node_t *node, int *length)
{
    int length2;
    xml_node_t *top, *child;

    /* 首先: 处理孩子节点: 选出下一个孩子节点 */

    if (NULL != node->temp)
    {
        child = node->temp;
        node->temp = child->next;
        node = child;
        return node;
    }

    /* 再次: 处理其兄弟节点: 选出下一个兄弟节点 */

    length2 = 0;

    /* 1. 弹出已经处理完成的节点 */
    top = stack_gettop(stack);
    if (xml_has_child(top))
    {
        /* sprintf(sp->ptr, "</%s>", top->name); */
        length2 += top->name.len + 3;
    }

    if (NULL == stack_pop(stack))
    {
        *length += length2;
        //log_error(xml->log, "Stack pop failed!");
        return NULL;
    }

    if (stack_empty(stack))
    {
        *length += length2;
        //log_error(xml->log, "Compelte fprint!");
        return NULL;
    }

    /* 2. 处理其下一个兄弟节点 */
    node = top->next;
    while (NULL == node)     /* 所有兄弟节点已经处理完成，说明父亲节点也处理完成 */
    {
        /* 3. 父亲节点出栈 */
        top = stack_pop(stack);
        if (NULL == top)
        {
            *length += length2;
            //log_error(xml->log, "Stack pop failed!");
            return NULL;
        }

        /* 4. 打印父亲节点结束标志 */
        if (xml_has_child(top))
        {
            /* sprintf(sp->ptr, "</%s>", top->name); */
            length2 += top->name.len+3;
        }

        if (stack_empty(stack))
        {
            *length += length2;
            return NULL;    /* 处理完成 */
        }

        /* 5. 选择父亲的兄弟节点 */
        node = top->next;
    }

    *length += length2;
    return node;
}

/******************************************************************************
 **函数名称: xml_pack_node_length
 **功    能: 计算节点打印成XML报文字串时的长度(注: XML无层次结构)
 **输入参数:
 **     root: XML树根节点
 **     stack: 栈
 **输出参数:
 **返    回: 节点及其属性、孩子节点的总长度
 **实现描述: 
 **注意事项: 
 **作    者: # Qifeng.zou # 2013.06.11 #
 ******************************************************************************/
int xml_pack_node_length(xml_tree_t *xml, xml_node_t *root, Stack_t *stack)
{
    int depth, length;
    xml_node_t *node = root;

    depth = stack_depth(stack);
    if (0 != depth)
    {
        //log_error(xml->log, "Stack depth must empty. depth:[%d]", depth);
        return XML_ERR_STACK;
    }

    length = 0;

    do
    {
        /* 1. 将要处理的节点压栈 */
        node->temp = node->child;
        
        if (stack_push(stack, node))
        {
            //log_error(xml->log, "Stack push failed!");
            return XML_ERR_STACK;
        }
        
        /* 2. 打印节点名 */
        depth = stack_depth(stack);
        
        xml_pack_name_length(node, length);
        
        /* 3. 打印属性节点 */
        if (xml_has_attr(node))
        {
            xml_pack_attr_length(node, length);
        }
        
        /* 4. 打印节点值 */
        xml_pack_value_length(node, length);
        
        /* 5. 选择下一个处理的节点: 从父亲节点、兄弟节点、孩子节点中 */
        node = xml_pack_next_length(xml, stack, node, &length);
        
    } while (NULL != node);

    if (!stack_empty(stack))
    {
        return XML_ERR_STACK;
    }

    return length;
}

/* 打印节点名(注: XML无层次格式) */
#define xml_pack_name(sp, node) \
{ \
    sprintf(sp->ptr, "<%s", node->name.str);\
    sp->ptr += (1+node->name.len);\
}

/* 打印属性节点(注: XML无层次格式) */
#define xml_pack_attr(sp, node) \
{ \
    while (NULL != node->temp) \
    { \
        if (xml_is_attr(node->temp)) \
        { \
            sprintf(sp->ptr, " %s=\"%s\"", node->temp->name.str, node->temp->value.str); \
            sp->ptr += (node->temp->name.len+node->temp->value.len+4); \
            node->temp = node->temp->next; \
            continue; \
        } \
        break; \
    } \
}

#if defined(__XML_PACK_CMARK__)
/* 打印节点值(注: XML无层次格式) */
#define xml_pack_value(sp, node) \
{ \
    if (xml_has_value(node)) \
    { \
        if (xml_has_child(node))  /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            sprintf(sp->ptr, ">%s", node->value.str); \
            sp->ptr += (node->value.len + 1); \
        } \
        else \
        { \
            sprintf(sp->ptr, ">%s</%s>", node->value.str, node->name.str); \
            sp->ptr += (node->value.len + node->name.len + 4); \
        } \
    } \
    else \
    { \
        if (NULL != node->temp)   /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            sprintf(sp->ptr, ">"); \
            sp->ptr += 1; \
        } \
        else \
        { \
            sprintf(sp->ptr, "></%s>", node->name.str); \
            sp->ptr += (node->name.len + 4); \
        } \
    } \
}
#else /*__XML_PACK_CMARK__*/
/* 打印节点值(注: XML无层次格式) */
#define xml_pack_value(sp, node) \
{ \
    if (xml_has_value(node)) \
    { \
        if (xml_has_child(node))  /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            sprintf(sp->ptr, ">%s", node->value.str); \
            sp->ptr += (node->value.len + 1); \
        } \
        else \
        { \
            sprintf(sp->ptr, ">%s</%s>", node->value.str, node->name.str); \
            sp->ptr += (node->value.len + node->name.len + 4); \
        } \
    } \
    else \
    { \
        if (NULL != node->temp)   /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            sprintf(sp->ptr, ">"); \
            sp->ptr += 1; \
        } \
        else \
        { \
            sprintf(sp->ptr, "/>"); \
            sp->ptr += 2; \
        } \
    } \
}
#endif /*__XML_PACK_CMARK__*/

/******************************************************************************
 **函数名称: xml_pack_next
 **功    能: 选择下一个处理的节点(注: XML无层次格式)
 **输入参数:
 **     root: 根节点
 **     stack: 栈
 **     sp: 缓存指针
 **输出参数:
 **返    回: 0: 成功  !0: 失败
 **实现描述: 
 **注意事项: 
 **     1. 刚开始时，栈的深度必须为1.
 **     2. 此时temp用来记录正在处理孩子节点
 **     3. 不打印root的兄弟节点
 **作    者: # Qifeng.zou # 2013.03.01 #
 ******************************************************************************/
static xml_node_t *xml_pack_next(
        xml_tree_t *xml, Stack_t *stack, xml_node_t *node, sprint_t *sp)
{
    xml_node_t *top, *child;

    if (NULL != node->temp)      /* 首先: 处理孩子节点: 选出下一个孩子节点 */
    {
        child = node->temp;
        node->temp = child->next;
        node = child;
        return node;
    }

    /* 再次: 处理其兄弟节点: 选出下一个兄弟节点 */

    /* 1. 弹出已经处理完成的节点 */
    top = stack_pop(stack);
    if (NULL == top)
    {
        //log_error(xml->log, "Stack pop failed!");
        return NULL;
    }

    if (xml_has_child(top))
    {
        sprintf(sp->ptr, "</%s>", top->name.str);
        sp->ptr += (top->name.len + 3);
    }

    if (stack_empty(stack))
    {
        //log_error(xml->log, "Compelte fprint!");
        return NULL;
    }

    /* 2. 处理其下一个兄弟节点 */
    node = top->next;
    while (NULL == node)     /* 所有兄弟节点已经处理完成，说明父亲节点也处理完成 */
    {
        /* 3. 父亲节点出栈 */
        top = stack_pop(stack);
        if (NULL == top)
        {
            //log_error(xml->log, "Stack pop failed!");
            return NULL;
        }

        /* 4. 打印父亲节点结束标志 */
        if (xml_has_child(top))
        {
            sprintf(sp->ptr, "</%s>", top->name.str);
            sp->ptr += (top->name.len + 3);
        }

        if (stack_empty(stack))
        {
            return NULL;    /* 处理完成 */
        }

        /* 5. 选择父亲的兄弟节点 */
        node = top->next;
    }

    return node;
}

/******************************************************************************
 **函数名称: xml_pack_tree
 **功    能: 将栈中XML节点的相关信息打印至缓存(注: XML无层次格式)
 **输入参数:
 **     root: 根节点
 **     stack: 栈
 **     sp: 缓存指针
 **输出参数:
 **返    回: 0: 成功  !0: 失败
 **实现描述: 
 **注意事项: 
 **     1. 刚开始时，栈的深度必须为1.
 **     2. 此时temp用来记录正在处理孩子节点
 **     3. 不打印root的兄弟节点
 **作    者: # Qifeng.zou # 2013.03.01 #
 ******************************************************************************/
int xml_pack_tree(xml_tree_t *xml, xml_node_t *root, Stack_t *stack, sprint_t *sp)
{
    int depth;
    xml_node_t *node = root;

    depth = stack_depth(stack);
    if (0 != depth)
    {
        //log_error(xml->log, "Stack depth must empty. depth:[%d]", depth);
        return XML_ERR_STACK;
    }

    do
    {
        /* 1. 将要处理的节点压栈 */
        node->temp = node->child;
        if (stack_push(stack, node))
        {
            //log_error(xml->log, "Stack push failed!");
            return XML_ERR_STACK;
        }
        
        /* 2. 打印节点名 */
        depth = stack_depth(stack);
        
        xml_pack_name(sp, node);
        
        /* 3. 打印属性节点 */
        if (xml_has_attr(node))
        {
            xml_pack_attr(sp, node);
        }
        
        /* 4. 打印节点值 */
        xml_pack_value(sp, node);
        
        /* 5. 选择下一个处理的节点: 从父亲节点、兄弟节点、孩子节点中 */
        node = xml_pack_next(xml, stack, node, sp);

    } while (NULL != node);

    if (!stack_empty(stack))
    {
        //log_error(xml->log, "Stack is not empty!");
        return XML_ERR_STACK;
    }
    return XML_OK;
}

/******************************************************************************
 **                                                                          **
 **                              打印成有层次结构                            **
 **                                 (内存中)                                 **
 ******************************************************************************/
/* 打印节点名(注: XML有层次格式) */
#define xml_sprint_name(sp, node, depth) \
{ \
    while (depth > 1) \
    {   \
        sprintf(sp->ptr, "\t"); sp->ptr++;\
        depth--; \
    } \
    sprintf(sp->ptr, "<%s", node->name.str);\
    sp->ptr += (node->name.len + 1);\
}

/* 打印属性节点(注: XML有层次格式) */
#define xml_sprint_attr(sp, node) \
{ \
    while (NULL != node->temp) \
    { \
        if (xml_is_attr(node->temp)) \
        { \
            sprintf(sp->ptr, " %s=\"%s\"", node->temp->name.str, node->temp->value.str); \
            sp->ptr += (node->temp->name.len + node->temp->value.len + 4); \
            node->temp = node->temp->next; \
            continue; \
        } \
        break; \
    } \
}

/* 打印节点值(注: XML有层次格式) */
#define xml_sprint_value(sp, node) \
{ \
    if (xml_has_value(node)) \
    { \
        if (xml_has_child(node))  /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            sprintf(sp->ptr, ">%s\n", node->value.str); \
            sp->ptr += (node->value.len + 2); \
        } \
        else \
        { \
            sprintf(sp->ptr, ">%s</%s>\n", node->value.str, node->name.str); \
            sp->ptr += (node->value.len + node->name.len + 5); \
        } \
    } \
    else \
    { \
        if (NULL != node->temp)   /* 此时temp指向node的孩子节点 或 NULL */ \
        { \
            sprintf(sp->ptr, ">\n"); \
            sp->ptr += 2; \
        } \
        else \
        { \
            sprintf(sp->ptr, "/>\n"); \
            sp->ptr += 3; \
        } \
    } \
}

/******************************************************************************
 **函数名称: xml_sprint_next
 **功    能: 选择下一个处理的节点(注: XML有层次格式)
 **输入参数:
 **     root: 根节点
 **     stack: 栈
 **     sp: 缓存指针
 **输出参数:
 **返    回: 0: 成功  !0: 失败
 **实现描述: 
 **注意事项: 
 **     1. 刚开始时，栈的深度必须为1.
 **     2. 此时temp用来记录正在处理孩子节点
 **     3. 不打印root的兄弟节点
 **作    者: # Qifeng.zou # 2013.02.27 #
 ******************************************************************************/
static xml_node_t *xml_sprint_next(
        xml_tree_t *xml, Stack_t *stack, xml_node_t *node, sprint_t *sp)
{
    int depth, level;
    xml_node_t *top, *child;

    if (NULL != node->temp)      /* 首先: 处理孩子节点: 选出下一个孩子节点 */
    {
        child = node->temp;
        node->temp = child->next;
        node = child;
        return node;
    }
    else                        /* 再次: 处理其兄弟节点: 选出下一个兄弟节点 */
    {
        /* 1. 弹出已经处理完成的节点 */
        top = stack_pop(stack);
        if (xml_has_child(top))
        {
            depth = stack_depth(stack);
            level = depth - 1;
            while (level > 1)
            {
                sprintf(sp->ptr, "\t");
                sp->ptr++;
                level--;
            }
            sprintf(sp->ptr, "</%s>\n", top->name.str);
            sp->ptr += (top->name.len + 4);
        }
        
        if (stack_empty(stack))
        {
            //log_error(xml->log, "Compelte fprint!");
            return NULL;
        }
        
        /* 2. 处理其下一个兄弟节点 */
        node = top->next;
        while (NULL == node)     /* 所有兄弟节点已经处理完成，说明父亲节点也处理完成 */
        {
            /* 3. 父亲节点出栈 */
            top = stack_pop(stack);
            if (NULL == top)
            {
                //log_error(xml->log, "Stack pop failed!");
                return NULL;
            }
        
            /* 4. 打印父亲节点结束标志 */
            if (xml_has_child(top))
            {
                depth = stack_depth(stack);
                level = depth + 1;
                while (level > 1)
                {
                    sprintf(sp->ptr, "\t");
                    sp->ptr++;
                    level--;
                }
                sprintf(sp->ptr, "</%s>\n", top->name.str);
                sp->ptr += (top->name.len + 4);
            }
            
            if (stack_empty(stack))
            {
                return NULL;    /* 处理完成 */
            }

            /* 5. 选择父亲的兄弟节点 */
            node = top->next;
        }
    }    
    
    return node;
}

/******************************************************************************
 **函数名称: xml_sprint_tree
 **功    能: 将栈中XML节点的相关信息打印至缓存(注: XML有层次格式)
 **输入参数:
 **     root: 根节点
 **     stack: 栈
 **     sp: 缓存指针
 **输出参数:
 **返    回: 0: 成功  !0: 失败
 **实现描述: 
 **注意事项: 
 **     1. 刚开始时，栈的深度必须为1.
 **     2. 此时temp用来记录正在处理孩子节点
 **     3. 不打印root的兄弟节点
 **作    者: # Qifeng.zou # 2013.02.27 #
 ******************************************************************************/
int xml_sprint_tree(xml_tree_t *xml, xml_node_t *root, Stack_t *stack, sprint_t *sp)
{
    int depth;
    xml_node_t *node = root;

    depth = stack_depth(stack);
    if (0 != depth)
    {
        //log_error(xml->log, "Stack depth must empty. depth:[%d]", depth);
        return XML_ERR_STACK;
    }

    do
    {
        /* 1. 将要处理的节点压栈 */
        node->temp = node->child;
        if (stack_push(stack, node))
        {
            //log_error(xml->log, "Stack push failed!");
            return XML_ERR_STACK;
        }
        
        /* 2. 打印节点名 */
        depth = stack_depth(stack);
        
        xml_sprint_name(sp, node, depth);
        
        /* 3. 打印属性节点 */
        if (xml_has_attr(node))
        {
            xml_sprint_attr(sp, node);
        }
        
        /* 4. 打印节点值 */
        xml_sprint_value(sp, node);
        
        /* 5. 选择下一个处理的节点: 从父亲节点、兄弟节点、孩子节点中 */
        node = xml_sprint_next(xml, stack, node, sp);

    } while(NULL != node);

    if (!stack_empty(stack))
    {
        //log_error(xml->log, "Stack is not empty!");
        return XML_ERR_STACK;
    }

    return XML_OK;
}
