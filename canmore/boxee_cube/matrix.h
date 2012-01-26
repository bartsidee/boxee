//-----------------------------------------------------------------------------
// Copyright (c) 2008 Intel Corporation
//
// DISTRIBUTABLE AS SAMPLE SOURCE SOFTWARE
//
// This Distributable As Sample Source Software is subject to the terms and
// conditions of the Intel Software License Agreement provided with the Intel(R)
// Media Processor Software Development Kit.
//------------------------------------------------------------------------------
/*-----------------------------------------------------------------------------
* matrix.c calculates all kinds of transformation matrices for OpenGL ES 2.0.
* For most functions, the first parameter m[16] is a column major output 
* matrix. The formulas used to calculate the transformation matrices and 
* projection matrices are based on the OpenGL ES specifications.
------------------------------------------------------------------------------*/
#ifndef MATRIX_H
#define MATRIX_H

/**
 * Normalize the vector to become a unit vector
 * Input: x, y, z --- input vector's x, y and z value
 * Output: fout[3]  --- output vector 
 */
float normalize(float fout[3], float x, float y, float z);

/**
 * Calculate the cross product of two input vectors
 * Input: src1[3], src2[3]  --- two input vectors
 * Output: fout[3]  --- output vector     
 */
void crossProduct(float fout[3], float src1[3], float src2[3]);

/**
 * Set the 4x4 matrix m[16] to the identity matrix
 */
void myIdentity(float m[16]);

/**
 * Multiply two 4x4 matrices
 * Input: src1[16], src2[16]  --- two source matrices
 * Output: m[16]              --- result matrix
 */
void myMultMatrix(float m[16], float src1[16], float src2[16]);

/**
 * Translate the 3D object in the scene
 * Input: tx , ty, tz    --- the offset for translation in X, Y, and Z dirctions 
 * Output: m[16]   --- the matrix after the translate
 */
void myTranslate(float m[16], float tx, float ty, float tz);

/**
 * Scale operation for the object in the scene 
 * Input: sx, sy, sz  --- scaling factors for X, Y and Z directions
 * Output: m[16]  --- the matrix after the scaling
 */
void myScale(float m[16], float sx, float sy, float sz);

/**
 * Rotation operation
 * Input: angle, x, y, z  (rotating angle and rotating axis)
 * Output: m[16]  --- the matrix after the rotation
 */
void myRotate(float m[16], float angle, float x, float y, float z);

/**
 * Perspective projection transformation
 * Input: left, right, bottom, top, near, far parameters similar to glFrustum 
 *        function
 * Output: m[16] ---  perspective projection matrix
 */
void myFrustum(float m[16], float left, float right, float bottom, float top, 
               float near, float far);

/**
 * Orthogonal projection transformation
 * Input: left, right, bottom, top, near, far parameters similar to glOrtho 
 *        function
 * Output: m[16]  4x4 projection matrix
 */
void myOrtho(float m[16], float left, float right, float bottom, float top, 
             float near, float far);

/**
 * Calculate the inverse tanspose matrix of a 4x4 matrix, and take the top 
 * left 3x3 matrix as output matrix.
 * Input: src[16]   4x4 matrix
 * Output: m[9]     3x3 matrix
 */
void myTransposeInvertMatrix(float m[16], float src[16]);

/**
 * Calculate the inverse matrix of a 4x4 matrix
 * Input: src[16]   4x4 matrix   
 * Output: m[16]    4x4 matrix
 */
void myInverseMatrix(float m[16], float src[16]);

/**
 * Similar to the gluLookAt function.
 * Input: eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ
 * Output: view matrix -- m[16]
 */
void myLookAt(float m[16], float eyeX, float eyeY, float eyeZ, float centerX, 
              float centerY, float centerZ, float upX, float upY, float upZ);

#endif
