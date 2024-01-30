#ifndef SHADER_NODES_H
#define SHADER_NODES_H

#include "raylib.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// @NOTE changes here need to be reflected elsewhere in this file
//       search term `shader_node_types_data`
typedef enum {
    SHADER_NODE_NONE,
    
    SHADER_NODE_OBJECT,
    
    SHADER_NODE_OP_UNION,
    SHADER_NODE_OP_INTERSECTION,
    SHADER_NODE_OP_SUBTRACTION,
    
    SHADER_NODE_OP_SMOOTH,
    
    SHADER_NODE_OP_TRANSLATION,
    SHADER_NODE_OP_ROTATION,
    SHADER_NODE_OP_SCALE,
    
    SHADER_NODE_PLANE,
    SHADER_NODE_BOX,
    SHADER_NODE_SPHERE,
    SHADER_NODE_ELLIPSOID,
    SHADER_NODE_CAPSULE,
    SHADER_NODE_CYLINDER,
    
    SHADER_NODE_MAX
} ShaderNodeType;

typedef enum {
    SHADER_NODE_TYPE_NONE,
    SHADER_NODE_TYPE_OBJECT,
    SHADER_NODE_TYPE_OPERATION,
    SHADER_NODE_TYPE_PRIMITIVE,
    SHADER_NODE_TYPE_MAX
} ShaderNodeTypeCategory;

extern const ShaderNodeTypeCategory shaderNodeTypeCategory[SHADER_NODE_MAX+1];

typedef struct ShaderNodeNav {
    int depth;
    int parent;
    int firstChild;
    int nextSibling;
    int prevSibling;
    int descendantCount;
} ShaderNodeNav;

typedef struct ShaderNode {
    ShaderNodeType type;
    int childNodeCount;
    
    unsigned char data[sizeof(float) * 12];
} ShaderNode;

typedef struct ShaderNodePrimitive {
    ShaderNodeType type;
    int childNodeCount;
    
    union {
        struct {
            Vector3 centre;
            union {
                Vector3 size;
                Vector3 normal;
            };
        };
        // For SHADER_NODE_CAPSULE, SHADER_NODE_CYLINDER
        struct {
            Vector3 p1;
            Vector3 p2;
            float radius;
        };
    };
} ShaderNodePrimitive;

typedef struct ShaderNodeOperation {
    ShaderNodeType type;
    int childNodeCount;
    
    union {
        float smoothness;       // For SHADER_NODE_OP_UNION, SHADER_NODE_OP_INTERSECTION and SHADER_NODE_OP_SUBTRACTION
        float scale;            // For SHADER_NODE_OP_SCALE
        Vector3 translation;
        Quaternion rotation;
    };
} ShaderNodeOperation;

typedef struct ShaderNodeObject {
    ShaderNodeType type;
    int childNodeCount;
    
    Color colDiffuse;
} ShaderNodeObject;

typedef struct ShaderWriterData {
    int codeHeaderLength;
    char *codeHeader;
    
    int codeFooterLength;
    char *codeFooter;
} ShaderWriterData;

extern ShaderWriterData shaderWriterData;

extern int shaderWriterError;
extern int shaderWriterErrorNode;
extern char shaderWriterErrorDesc[1024];

// @TODO allow this global stuff to be optional
#ifndef SHADER_NODES_MAX
    #define SHADER_NODES_MAX 4096
#endif

extern ShaderNode shaderNodes[SHADER_NODES_MAX];
extern ShaderNodeNav shaderNodesNav[SHADER_NODES_MAX];
extern int shaderNodeCount;

void InitShaderWriter(ShaderWriterData *data);
void FreeShaderWriter(ShaderWriterData *data);
int WriteShader(ShaderWriterData *data, ShaderNode *nodes, int nodeCount, void *output, int outputCapacity);
int GetShaderNodeChild(ShaderNode *nodes, int nodeCount, int parent, int wantedChildIndex);
int GetShaderNodeParent(ShaderNode *nodes, int nodeCount, int child);

void RecalcShaderNodesNav(ShaderNode *nodes, ShaderNodeNav *navNodes, int nodeCount);

ShaderNode MakeShaderNodePrimitive(ShaderNodeType type, Vector3 centre, Vector3 size);
ShaderNode MakeShaderNodeOperation(ShaderNodeType type, int childCount, float smoothness);

#endif

#ifdef SHADER_NODES_IMPLEMENTATION

ShaderWriterData shaderWriterData = { 0 };

ShaderNode shaderNodes[SHADER_NODES_MAX];
ShaderNodeNav shaderNodesNav[SHADER_NODES_MAX];
int shaderNodeCount = 0;

// @NOTE to be synced with ShaderNodeType [shader_node_types_data]
const char *nodeTypeNames[] = {
    "NODE_NONE",
    "NODE_OBJECT",
    "NODE_OP_UNION",
    "NODE_OP_INTERSECTION",
    "NODE_OP_SUBTRACTION",
    "NODE_OP_SMOOTH",
    "NODE_OP_TRANSLATION",
    "NODE_OP_ROTATION",
    "NODE_OP_SCALE",
    "NODE_PLANE",
    "NODE_BOX",
    "NODE_SPHERE",
    "NODE_ELLIPSOID",
    "NODE_CAPSULE",
    "NODE_CYLINDER",
    "NODE_MAX"
};

// @NOTE to be synced with ShaderNodeType [shader_node_types_data]
const ShaderNodeTypeCategory shaderNodeTypeCategory[SHADER_NODE_MAX+1] = {
    SHADER_NODE_TYPE_NONE,
    SHADER_NODE_TYPE_OBJECT,
    SHADER_NODE_TYPE_OPERATION,
    SHADER_NODE_TYPE_OPERATION,
    SHADER_NODE_TYPE_OPERATION,
    SHADER_NODE_TYPE_OPERATION,
    SHADER_NODE_TYPE_OPERATION,
    SHADER_NODE_TYPE_OPERATION,
    SHADER_NODE_TYPE_OPERATION,
    SHADER_NODE_TYPE_PRIMITIVE,
    SHADER_NODE_TYPE_PRIMITIVE,
    SHADER_NODE_TYPE_PRIMITIVE,
    SHADER_NODE_TYPE_PRIMITIVE,
    SHADER_NODE_TYPE_PRIMITIVE,
    SHADER_NODE_TYPE_PRIMITIVE,
    SHADER_NODE_TYPE_MAX
};

typedef struct {
    int capacity;
    void *data;
    unsigned char *writePointer;
    int written;
} ShaderWriterBuffer;

static int WriteShaderNode(ShaderNode *nodes, int nodeCount, int nodeIndex, ShaderWriterBuffer *buffer, int stackDepth, char *posExpression);

void InitShaderWriter(ShaderWriterData *data) {
    #define CHECK_NODE_TYPE_SIZE(nodeTypeName, nodeType) {\
        TraceLog(\
            (sizeof(nodeType)>sizeof(ShaderNode)) ? LOG_ERROR : LOG_INFO,\
            "Size of '" nodeTypeName "' is %d, ShaderNode = %d", sizeof(nodeType), sizeof(ShaderNode)\
        ); }
        
    CHECK_NODE_TYPE_SIZE("ShaderNodePrimitive", ShaderNodePrimitive);
    CHECK_NODE_TYPE_SIZE("ShaderNodeOperation", ShaderNodeOperation);
    CHECK_NODE_TYPE_SIZE("ShaderNodeObject", ShaderNodeObject);
    
    data->codeHeader = (char *)LoadFileData("shader_writer_header.txt", &data->codeHeaderLength);
    data->codeFooter = (char *)LoadFileData("shader_writer_footer.txt", &data->codeFooterLength);
}

void FreeShaderWriter(ShaderWriterData *data) {
    UnloadFileData((unsigned char*)data->codeHeader); data->codeHeader = 0; data->codeHeaderLength = 0;
    UnloadFileData((unsigned char*)data->codeFooter); data->codeFooter = 0; data->codeFooterLength = 0;
}

int shaderWriterError = 0;
int shaderWriterErrorNode = -1;
char shaderWriterErrorDesc[1024] = {0};

#define DEBUG_WRITE_SHADER 1

static int WriteToBuffer(ShaderWriterBuffer *buffer, void *dataPointer, int dataSize, const char *stageForDebug) {
    if (!buffer->capacity == 0) {
        if ((buffer->capacity-1-buffer->written) >= dataSize) {
            memcpy(buffer->writePointer, dataPointer, dataSize);
            if (false) {
                TraceLog(LOG_INFO, "Wrote %d bytes to shader (stage: '%s', ptr = %x, capacity: %d (/%d total)", dataSize, stageForDebug, buffer->writePointer, buffer->capacity-1-buffer->written-dataSize, buffer->capacity);
            }
            buffer->writePointer += dataSize;
        }
        else {
            shaderWriterError = 1;
            sprintf(shaderWriterErrorDesc, "Cannot write shader, out of buffer space (stage: '%s'), need to write: %d, capacity: %d (/%d total)", stageForDebug, dataSize, buffer->capacity-1-buffer->written, buffer->capacity); 
            TraceLog(LOG_ERROR, shaderWriterErrorDesc);
            return 0;
        }
    }
    buffer->written += dataSize;
    
    return 1;
}

int WriteShader(ShaderWriterData *data, ShaderNode *nodes, int nodeCount, void *outputBuffer, int outputBufferCapacity) {
    
    shaderWriterError = 0;
    shaderWriterErrorNode = -1;
    shaderWriterErrorDesc[0] = '\0';
    
    if (nodeCount == 0) {
        shaderWriterError = -1;
        sprintf(shaderWriterErrorDesc, "Cannot write shader, no nodes to write");
        TraceLog(LOG_ERROR, shaderWriterErrorDesc);
        return 0;
    }
    
    ShaderWriterBuffer buffer = {
        .capacity = outputBufferCapacity,
        .data = outputBuffer,
        .writePointer = outputBuffer,
        .written = 0
    };
    
    #define WRITE_TO_BUFFER(dataPointer, dataSize, stage) if (!WriteToBuffer(&buffer, dataPointer, dataSize, stage)) { return 0; }
    
    WRITE_TO_BUFFER(data->codeHeader, data->codeHeaderLength, "Header");
    
    // --------
    
    // @TODO can we have more than one node at the root? hmmm
    if (!WriteShaderNode(nodes, nodeCount, 0, &buffer, 0, "pos")) {
        TraceLog(LOG_ERROR, "Error writing nodes; hopefully they gave more info");
        return 0;
    }
    
    // --------
    
    WRITE_TO_BUFFER(data->codeFooter, data->codeFooterLength, "Footer");
    
    /*
    if (!calcSizeMode) {
        for (int i = written-8; i<written+4; i++) {
            TraceLog(LOG_INFO, "Address %i : %02x (%c)", i, (int)((unsigned char*)outputBuffer)[i], ((unsigned char*)outputBuffer)[i] >= 32 ? ((unsigned char*)outputBuffer)[i] : '#');
        }
    }
    */
    
    WRITE_TO_BUFFER("\0", 1, "Terminator");
    
    #undef INDENT
    #undef WRITE_TO_BUFFER
    
    return buffer.written;
}



static int WriteShaderNode(ShaderNode *nodes, int nodeCount, int nodeIndex, ShaderWriterBuffer *buffer, int stackDepth, char *posExpression) {
    #define WRITE_TO_BUFFER(dataPointer, dataSize, stage) if (!WriteToBuffer(buffer, dataPointer, dataSize, stage)) { return 0; }
    
    // ------------------
    
    static char spaces[1024] = { 0 };
    if (spaces[0] == 0) {
        memset(spaces, ' ', 1023);
        spaces[1023] = '\0';
    }
    
    #define INDENT(level) (&spaces[1023-((level)+1)*4])
    
    char newPosExpression[1024] = { 0 };
    
    char lineToWrite[4096];
    int lineToWriteLength = 0;

    ShaderNode *node = &nodes[nodeIndex];
    ShaderNodePrimitive* nodePrimitive = (ShaderNodePrimitive*)node;
    ShaderNodeOperation* nodeOperation = (ShaderNodeOperation*)node;
    ShaderNodeObject *nodeObject = (ShaderNodeObject*)node;
    
    switch(node->type) {
        case SHADER_NODE_OBJECT:
            if (node->childNodeCount == 0) {
                // No children, so a pointless object
                lineToWriteLength = sprintf(lineToWrite, "%s// Skipping Node %d [empty object]\n, nodeIndex", INDENT(stackDepth), nodeIndex);
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                
                TraceLog(LOG_WARNING, "Node #%d : Degenerate object (%d children); skipping", nodeIndex, node->childNodeCount);
                break;
            }
            // Output the object
            
            lineToWriteLength = sprintf(lineToWrite, "%scolDiffuse = vec3(%.3f,%.3f,%.3f);\n%sres = addObject( res, vec2( 1.0, \n",
                INDENT(stackDepth), (float)nodeObject->colDiffuse.r/255.0f, (float)nodeObject->colDiffuse.g/255.0f, (float)nodeObject->colDiffuse.b/255.0f,
                INDENT(stackDepth));
            WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            
            // -----------------------
            
            // If there is more than one child, do an implicit union
            if (node->childNodeCount > 1) {
                stackDepth++;
                lineToWriteLength = sprintf(lineToWrite, "%sopU(", INDENT(stackDepth));
                for (int i = 0; i < nodeOperation->childNodeCount-2; i++) {
                    lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], "opU(");
                }
                lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], " /* Implicit union */");
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            }
            
            for (int i = 0; i < node->childNodeCount; i++) {
                if (node->childNodeCount > 1) {
                    lineToWriteLength = sprintf(lineToWrite, "\n%s/* implicit union child %d of %d */\n", INDENT(stackDepth+1), i+1, node->childNodeCount);
                    WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                }
                
                int child = GetShaderNodeChild(nodes, nodeCount, nodeIndex, i);
                int ret = WriteShaderNode(nodes, nodeCount, child, buffer, stackDepth+1, posExpression);
                if (!ret) {
                    return ret;
                }
                
                if (node->childNodeCount > 1) {
                    if (i == 0) {
                        lineToWriteLength = sprintf(lineToWrite, ",");
                    }
                    else if (i < node->childNodeCount-1) {
                        lineToWriteLength = sprintf(lineToWrite, "),");
                    }
                    else {
                        continue;
                    }
                    WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                }
            }
            if (node->childNodeCount > 1) {
                lineToWriteLength = sprintf(lineToWrite, "\n%s)", INDENT(stackDepth));
                lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], " /* End implicit union [smooth] */");
                
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                stackDepth--;
            }
            
            // -----------------------
            
            lineToWriteLength = sprintf(lineToWrite, "\n%s)); /* End object */\n", INDENT(stackDepth));
            WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            
            break;
        case SHADER_NODE_BOX:
            lineToWriteLength = sprintf(lineToWrite, "%ssdBox(%s - vec3(%.3f, %.3f, %.3f), vec3(%.3f, %.3f, %.3f))", INDENT(stackDepth),
                posExpression == 0 ? "pos" : posExpression,
                nodePrimitive->centre.x,
                nodePrimitive->centre.y,
                nodePrimitive->centre.z,
                nodePrimitive->size.x,
                nodePrimitive->size.y,
                nodePrimitive->size.z
            );
            WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            if (node->childNodeCount > 1) {
                TraceLog(LOG_WARNING, "Node #%s of type %d has children; ignoring them", nodeIndex, nodeTypeNames[node->type]);
            }
            break;
        case SHADER_NODE_SPHERE:
            lineToWriteLength = sprintf(lineToWrite, "%ssdSphere(%s - vec3(%.3f, %.3f, %.3f), %.3f)", INDENT(stackDepth),
                posExpression == 0 ? "pos" : posExpression,
                nodePrimitive->centre.x,
                nodePrimitive->centre.y,
                nodePrimitive->centre.z,
                nodePrimitive->size.x
            );
            WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            if (node->childNodeCount > 1) {
                TraceLog(LOG_WARNING, "Node #%s of type %d has children; ignoring them", nodeIndex, nodeTypeNames[node->type]);
            }
            break;
        case SHADER_NODE_ELLIPSOID:
            lineToWriteLength = sprintf(lineToWrite, "%ssdEllipsoid(%s - vec3(%.3f, %.3f, %.3f), vec3(%.3f,%.3f,%.3f))", INDENT(stackDepth),
                posExpression == 0 ? "pos" : posExpression,
                nodePrimitive->centre.x,
                nodePrimitive->centre.y,
                nodePrimitive->centre.z,
                nodePrimitive->size.x,
                nodePrimitive->size.y,
                nodePrimitive->size.z
            );
            WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            if (node->childNodeCount > 1) {
                TraceLog(LOG_WARNING, "Node #%s of type %d has children; ignoring them", nodeIndex, nodeTypeNames[node->type]);
            }
            break;
        case SHADER_NODE_CAPSULE:
            lineToWriteLength = sprintf(lineToWrite, "%ssdCapsule(%s, vec3(%.3f, %.3f, %.3f), vec3(%.3f, %.3f, %.3f), %.3f)", INDENT(stackDepth),
                posExpression == 0 ? "pos" : posExpression,
                nodePrimitive->p1.x,
                nodePrimitive->p1.y,
                nodePrimitive->p1.z,
                nodePrimitive->p2.x,
                nodePrimitive->p2.y,
                nodePrimitive->p2.z,
                nodePrimitive->radius
            );
            WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            if (node->childNodeCount > 1) {
                TraceLog(LOG_WARNING, "Node #%s of type %d has children; ignoring them", nodeIndex, nodeTypeNames[node->type]);
            }
            break;
        case SHADER_NODE_CYLINDER:
            lineToWriteLength = sprintf(lineToWrite, "%ssdCappedCylinder(%s, vec3(%.3f, %.3f, %.3f), vec3(%.3f, %.3f, %.3f), %.3f)", INDENT(stackDepth),
                posExpression == 0 ? "pos" : posExpression,
                nodePrimitive->p1.x,
                nodePrimitive->p1.y,
                nodePrimitive->p1.z,
                nodePrimitive->p2.x,
                nodePrimitive->p2.y,
                nodePrimitive->p2.z,
                nodePrimitive->radius
            );
            WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            if (node->childNodeCount > 1) {
                TraceLog(LOG_WARNING, "Node #%s of type %d has children; ignoring them", nodeIndex, nodeTypeNames[node->type]);
            }
            break;
        case SHADER_NODE_OP_UNION:
        case SHADER_NODE_OP_SUBTRACTION:
        case SHADER_NODE_OP_INTERSECTION:
            {
                const char *normalFnNames[] = {"opU", "opS", "opI"};
                const char *smoothFnNames[] = {"opSmoothUnion", "opSmoothSubtraction", "opSmoothIntersection"};
                const char *debugNames[]    = {"union", "subtraction", "intersection"};
                int selector = (node->type == SHADER_NODE_OP_UNION) * 1 + (node->type == SHADER_NODE_OP_SUBTRACTION) * 2 + (node->type == SHADER_NODE_OP_INTERSECTION) * 2 - 1;
                const char *normalFnName = normalFnNames[selector];
                const char *smoothFnName = smoothFnNames[selector];
                const char *debugName    = debugNames[selector];
                
                
                if (node->childNodeCount == 0) {
                    //lineToWriteLength = sprintf(lineToWrite, "%s// Skipping Node %d [empty union]\n, nodeIndex", INDENT(stackDepth), nodeIndex);
                    lineToWriteLength = sprintf(lineToWrite, "%s// Skipping Node %d [empty %s]\n, nodeIndex", INDENT(stackDepth), nodeIndex, debugName);
                    WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                    
                    TraceLog(LOG_WARNING, "Node #%d : Degenerate %s (%d children); skipping", nodeIndex, debugName, node->childNodeCount);
                    break;
                }
                
                if (node->childNodeCount == 1) {
                    // Only one child - degenerate but ok
                    // @TODO is this correct for subtraction?
                    TraceLog(LOG_WARNING, "Node #%d : Degenerate %s (%d children); writing child directly", nodeIndex, debugName, node->childNodeCount);
                    return WriteShaderNode(nodes, nodeCount, nodeIndex+1, buffer, stackDepth+1, posExpression);
                }
                

                // Build up the stack of union calls
                if (nodeOperation->smoothness == 0.0f) {
                    lineToWriteLength = sprintf(lineToWrite, "%s%s(", INDENT(stackDepth), normalFnName);
                    for (int i = 0; i < nodeOperation->childNodeCount-2; i++) {
                        lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], "%s(", normalFnName);
                    }
                }
                else {
                    lineToWriteLength = sprintf(lineToWrite, "%s%s(%.3f, ", INDENT(stackDepth), smoothFnName, nodeOperation->smoothness);
                    for (int i = 0; i < nodeOperation->childNodeCount-2; i++) {
                        lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], "%s(%.3f, ", smoothFnName, nodeOperation->smoothness);
                    }
                }
                
                //lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], "\n");
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                
                // Write the children
                for (int i = 0; i < node->childNodeCount; i++) {
                    
                    lineToWriteLength = sprintf(lineToWrite, "\n%s/* %s child %d of %d */\n", INDENT(stackDepth+1), debugName, i+1, node->childNodeCount);
                    WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                    
                    int child = GetShaderNodeChild(nodes, nodeCount, nodeIndex, i);
                    int ret = WriteShaderNode(nodes, nodeCount, child, buffer, stackDepth+1, posExpression);
                    if (!ret) {
                        return ret;
                    }
                    if (i == 0) {
                        lineToWriteLength = sprintf(lineToWrite, ",");
                    }
                    else if (i < node->childNodeCount-1) {
                        lineToWriteLength = sprintf(lineToWrite, "),");
                    }
                    else {
                        continue;
                    }
                    WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                }
                
                // Close the stack
                lineToWriteLength = sprintf(lineToWrite, "\n%s)", INDENT(stackDepth));
                lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], " /* End %s */", debugName);
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            }
            break;
        
        case SHADER_NODE_OP_SMOOTH:
        
            if (node->childNodeCount == 0) {
                //lineToWriteLength = sprintf(lineToWrite, "%s// Skipping Node %d [empty smooth]\n, nodeIndex", INDENT(stackDepth), nodeIndex);
                lineToWriteLength = sprintf(lineToWrite, "%s/* [skipping %d - empty smooth] */ (9999.99)\n", INDENT(stackDepth), nodeIndex);
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                
                TraceLog(LOG_WARNING, "Node #%d : Degenerate smooth (%d children); skipping", nodeIndex, node->childNodeCount);
                break;
            }
            
            lineToWriteLength = sprintf(lineToWrite, "%sopSmooth(%.3f, \n", INDENT(stackDepth), nodeOperation->smoothness);
            WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));

            // If there is more than one child, do an implicit union
            if (node->childNodeCount > 1) {
                stackDepth++;
                lineToWriteLength = sprintf(lineToWrite, "%sopU(", INDENT(stackDepth));
                for (int i = 0; i < nodeOperation->childNodeCount-2; i++) {
                    lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], "opU(");
                }
                lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], " /* Implicit union */");
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            }
            
            for (int i = 0; i < node->childNodeCount; i++) {
                if (node->childNodeCount > 1) {
                    lineToWriteLength = sprintf(lineToWrite, "\n%s/* implicit union child %d of %d */\n", INDENT(stackDepth+1), i+1, node->childNodeCount);
                    WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                }
                
                int child = GetShaderNodeChild(nodes, nodeCount, nodeIndex, i);
                int ret = WriteShaderNode(nodes, nodeCount, child, buffer, stackDepth+1, posExpression);
                if (!ret) {
                    return ret;
                }
                
                if (node->childNodeCount > 1) {
                    if (i == 0) {
                        lineToWriteLength = sprintf(lineToWrite, ",");
                    }
                    else if (i < node->childNodeCount-1) {
                        lineToWriteLength = sprintf(lineToWrite, "),");
                    }
                    else {
                        continue;
                    }
                    WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                }
            }
            if (node->childNodeCount > 1) {
                lineToWriteLength = sprintf(lineToWrite, "\n%s)", INDENT(stackDepth));
                lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], " /* End implicit union [smooth] */");
                
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                stackDepth--;
            }
            
            lineToWriteLength = sprintf(lineToWrite, "\n%s)", INDENT(stackDepth));
            WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            
            break;
            
        
        case SHADER_NODE_OP_TRANSLATION:
        
            if (node->childNodeCount == 0) {
                //lineToWriteLength = sprintf(lineToWrite, "%s// Skipping Node %d [empty translation]\n", INDENT(stackDepth), nodeIndex);
                lineToWriteLength = sprintf(lineToWrite, "%s/* [empty translation] */ (9999.99)\n", INDENT(stackDepth));
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                
                TraceLog(LOG_WARNING, "Node #%d : Degenerate translation (%d children); skipping", nodeIndex, node->childNodeCount);
                break;
            }
            
            sprintf(newPosExpression, "((%s)-vec3(%.3f,%.3f,%.3f))", posExpression == 0 ? "pos" : posExpression,
                nodeOperation->translation.x,
                nodeOperation->translation.y,
                nodeOperation->translation.z
            );

            // If there is more than one child, do an implicit union
            if (node->childNodeCount > 1) {
                stackDepth++;
                lineToWriteLength = sprintf(lineToWrite, "%sopU(", INDENT(stackDepth));
                for (int i = 0; i < nodeOperation->childNodeCount-2; i++) {
                    lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], "opU(");
                }
                lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], " /* Implicit union */");
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            }
            
            for (int i = 0; i < node->childNodeCount; i++) {
                if (node->childNodeCount > 1) {
                    lineToWriteLength = sprintf(lineToWrite, "\n%s/* implicit union child %d of %d */\n", INDENT(stackDepth+1), i+1, node->childNodeCount);
                    WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                }
                
                int child = GetShaderNodeChild(nodes, nodeCount, nodeIndex, i);
                int ret = WriteShaderNode(nodes, nodeCount, child, buffer, stackDepth+1, newPosExpression);
                if (!ret) {
                    return ret;
                }
                
                if (node->childNodeCount > 1) {
                    if (i == 0) {
                        lineToWriteLength = sprintf(lineToWrite, ",");
                    }
                    else if (i < node->childNodeCount-1) {
                        lineToWriteLength = sprintf(lineToWrite, "),");
                    }
                    else {
                        continue;
                    }
                    WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                }
            }
            if (node->childNodeCount > 1) {
                lineToWriteLength = sprintf(lineToWrite, "\n%s)", INDENT(stackDepth));
                lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], " /* End implicit union [translate] */");
                
                //WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                stackDepth--;
            }
            
            //lineToWriteLength = sprintf(lineToWrite, "\n%s)", INDENT(stackDepth));
            WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            
            break;
        case SHADER_NODE_OP_ROTATION:
        
            if (node->childNodeCount == 0) {
                lineToWriteLength = sprintf(lineToWrite, "%s/* [empty rotation] */ (9999.99)\n", INDENT(stackDepth));
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                
                TraceLog(LOG_WARNING, "Node #%d : Degenerate rotation (%d children); skipping", nodeIndex, node->childNodeCount);
                break;
            }
            
            Quaternion q = QuaternionFromAxisAngle((Vector3){nodeOperation->rotation.x, nodeOperation->rotation.y, nodeOperation->rotation.z}, nodeOperation->rotation.w);
            Matrix m = QuaternionToMatrix(q);
            
            sprintf(newPosExpression, "(mat3(%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f)*(%s))",
                m.m0, m.m4, m.m8,
                m.m1, m.m5, m.m9,
                m.m2, m.m6, m.m10,
                posExpression == 0 ? "pos" : posExpression
            );

            // If there is more than one child, do an implicit union
            if (node->childNodeCount > 1) {
                stackDepth++;
                lineToWriteLength = sprintf(lineToWrite, "%sopU(", INDENT(stackDepth));
                for (int i = 0; i < nodeOperation->childNodeCount-2; i++) {
                    lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], "opU(");
                }
                lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], " /* Implicit union */");
                WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            }
            
            for (int i = 0; i < node->childNodeCount; i++) {
                if (node->childNodeCount > 1) {
                    lineToWriteLength = sprintf(lineToWrite, "\n%s/* implicit union child %d of %d */\n", INDENT(stackDepth+1), i+1, node->childNodeCount);
                    WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                }
                
                int child = GetShaderNodeChild(nodes, nodeCount, nodeIndex, i);
                int ret = WriteShaderNode(nodes, nodeCount, child, buffer, stackDepth+1, newPosExpression);
                if (!ret) {
                    return ret;
                }
                
                if (node->childNodeCount > 1) {
                    if (i == 0) {
                        lineToWriteLength = sprintf(lineToWrite, ",");
                    }
                    else if (i < node->childNodeCount-1) {
                        lineToWriteLength = sprintf(lineToWrite, "),");
                    }
                    else {
                        continue;
                    }
                    WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                }
            }
            if (node->childNodeCount > 1) {
                lineToWriteLength = sprintf(lineToWrite, "\n%s)", INDENT(stackDepth));
                lineToWriteLength += sprintf(&lineToWrite[lineToWriteLength], " /* End implicit union [rotation] */");
                
                //WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
                stackDepth--;
            }
            
            //lineToWriteLength = sprintf(lineToWrite, "\n%s)", INDENT(stackDepth));
            WRITE_TO_BUFFER(lineToWrite, lineToWriteLength, TextFormat("Node %d", nodeIndex));
            
            break;
            
        default:
            TraceLog(LOG_WARNING, "Node #%d : Unhandled (type %s); skipping", nodeIndex, nodeTypeNames[node->type]);
    }
    
    // ------------------
    
    #undef INDENT
    #undef WRITE_TO_BUFFER
    
    return buffer->written;
}

// @TODO this is horrendously complicated for something so simple
// better data structure required!!
int GetShaderNodeChild(ShaderNode *nodes, int nodeCount, int parent, int wantedChildIndex) {
    if (parent < 0 || parent >= nodeCount) return -1;
    
    if (nodes[parent].childNodeCount == 0) return -1;
    if (wantedChildIndex < 0 || wantedChildIndex >= nodes[parent].childNodeCount) return -1;
    
    int childIndex = -1;
    int skipCount = 0;
    for (int i = parent+1; i < nodeCount; i++) {
        if (skipCount > 0) {
            // Not a direct child
            skipCount--;
        }
        else {
            // got a direct child
            childIndex++;
            // check we're still good
            if (childIndex >= nodes[parent].childNodeCount) {
                break;
            }
            // ok, handle the child here:
            if (childIndex == wantedChildIndex) {
                return i;
            }
        }
        
        if (nodes[i].childNodeCount > 0) {
            skipCount += nodes[i].childNodeCount;
        }
    }
    
    return -1;
}

void RecalcShaderNodesNav(ShaderNode *nodes, ShaderNodeNav *navNodes, int nodeCount) {
    #define DEBUG_TREE(pass, next) {\
        /*TraceLog(LOG_INFO, "Pass %d, next ref = #%d", pass, next);*/\
        TraceLog(LOG_INFO, "ID : \t Depth \t Parent \t Prev \t Next \t Descendants");\
        for (int i = 0; i < nodeCount; i++) {\
            TraceLog(LOG_INFO, "#%d : \t %d \t %d \t %d \t %d \t %d", i, navNodes[i].depth, navNodes[i].parent, navNodes[i].prevSibling, navNodes[i].nextSibling, navNodes[i].descendantCount);\
        }\
    }\
    
    navNodes[0] = (ShaderNodeNav){ .depth = 0, .parent = -1, .prevSibling = -1, .nextSibling = -1 };
    
    // Phase 1 : Calculate the depths
    {
        for (int i = 0; i < nodeCount; i++) {
            shaderNodesNav[i].depth = -1;
        }
        
        shaderNodesNav[0].depth = 0;
        
        static int stackParents[1024];
        static int stackRemainingChildren[1024];
        
        stackParents[0] = -1;
        stackRemainingChildren[0] = 999;
        
        int stackDepth = 0;
        
        for (int i = 0; i < nodeCount; i++) {
            shaderNodesNav[i].depth = stackDepth;
            shaderNodesNav[i].parent = stackParents[stackDepth];
            
            if (stackDepth > 0) {
                stackRemainingChildren[stackDepth]--;
            }
            
            if (shaderNodes[i].childNodeCount > 0) {
                stackDepth++;
                stackParents[stackDepth] = i;
                stackRemainingChildren[stackDepth] = shaderNodes[i].childNodeCount;
            }
            else if (stackDepth > 0) {
                while (stackDepth > 0 && stackRemainingChildren[stackDepth] == 0) {
                    // That was the last child; pop the stack
                    stackDepth--;
                }
            }
        }
    }
    // Phase 2 : Calculate the descendant count
    {
        for (int i = 0; i < nodeCount; i++) {
            navNodes[i].descendantCount = 0;
        }
        
        for (int i = 0; i < nodeCount; i++) {
            int j = navNodes[i].parent;
            while (j != -1) {
                navNodes[j].descendantCount++;
                j = navNodes[j].parent;
            }
        }
    }
    // Phase 3 : prev and next
    {
        for (int i = 0; i < nodeCount; i++) {
            int next = i + navNodes[i].descendantCount + 1;
            if (next >= nodeCount) next = -1;
            if (next != -1 && navNodes[next].parent != navNodes[i].parent) next = -1;
            navNodes[i].nextSibling = next;
            
        }
        for (int i = 0; i < nodeCount; i++) {
            if (navNodes[i].nextSibling != -1) {
                navNodes[ navNodes[i].nextSibling ].prevSibling = i;
            }
        }
    }
    
    DEBUG_TREE(0, 0);
    
    TraceLog(LOG_INFO, "Finished `RecalcShaderNodesNav`");
    
}

ShaderNode MakeShaderNodePrimitive(ShaderNodeType type, Vector3 centre, Vector3 size) {
    ShaderNode node = {type};
    ShaderNodePrimitive *nodePrimitive = (ShaderNodePrimitive*)&node;
    nodePrimitive->centre = centre;
    nodePrimitive->size = size;
    return node;
}

ShaderNode MakeShaderNodeOperation(ShaderNodeType type, int childCount, float smoothness) {
    ShaderNode node = {type};
    ShaderNodeOperation *nodeOperation = (ShaderNodeOperation*)&node;
    nodeOperation->childNodeCount = childCount;
    nodeOperation->smoothness = smoothness;
    return node;
}

#endif