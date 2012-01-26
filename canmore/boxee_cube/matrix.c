//-----------------------------------------------------------------------------
// Copyright (c) 2008 Intel Corporation
//
// DISTRIBUTABLE AS SAMPLE SOURCE SOFTWARE
//
// This Distributable As Sample Source Software is subject to the terms and
// conditions of the Intel Software License Agreement provided with the Intel(R)
// Media Processor Software Development Kit.
//------------------------------------------------------------------------------
/*------------------------------------------------------------------------------
 * matrix.c calculates all kinds of transformation matrices for OpenGL ES 2.0.
 * For most functions, the first parameter m[16] is a column major output 
 * matrix. The formulas used to calculate the transformation matrices and 
 * projection matrices are based on the OpenGL ES specifications.
 -----------------------------------------------------------------------------*/ 
#include <math.h>
#include <stdio.h>
#include "matrix.h"

/**
 * Normalize the vector to become a unit vector
 * Input: x, y, z --- input vector's x, y and z value
 * Output: fout[3]  --- output vector 
 */ 
float normalize(float fout[3], float x, float y, float z)
{
    float len = 0;    
    float lenSquare = 0;

    if (fout != NULL)
    {    
        lenSquare = x*x + y*y + z*z;

        if (lenSquare == 0)
        {
            fprintf(stderr,"normalize() error: vector length is zero.\n");
        }
        else
        {
            len = 1.0 / sqrt(lenSquare);
           
            fout[0] = x * len;
            fout[1] = y * len;
            fout[2] = z * len;
        }
    }
    
    return lenSquare;
}

/**
 * Set the 4x4 matrix m[16] to the identity matrix
 */
void myIdentity(float m[16])
{
    int i;
    if (m != NULL)
    {
        for (i=0; i<16; i++)
        {
            m[i] = 0;
        }
    
        m[0] = 1.0;
        m[5] = 1.0; 
        m[10] = 1.0;
        m[15] = 1.0;
    }
    else
    {
        fprintf(stderr, "myIdentity() error: matrix pointer is null.\n");
    }
}

/**
 * Multiply two 4x4 matrices
 * Input: src1[16], src2[16]  --- two source matrices
 * Output: m[16]              --- result matrix
 */
void myMultMatrix(float m[16], float src1[16], float src2[16])
{
    /* src1 or src2 could be the result matrix m as well, so use an intermediate
       matrix tm[16] to store the result */
    float tm[16];
    int i = 0;

    if ( m != NULL 
         && src1 != NULL
         && src2 != NULL )
    {
        tm[0] =   src1[0] * src2[0] + src1[4] * src2[1] 
                + src1[8] * src2[2] + src1[12]* src2[3];
        tm[1] =   src1[1] * src2[0] + src1[5] * src2[1] 
                + src1[9] * src2[2] + src1[13]* src2[3];
        tm[2] =   src1[2] * src2[0] + src1[6] * src2[1] 
                + src1[10]* src2[2] + src1[14]* src2[3];
        tm[3] =   src1[3] * src2[0] + src1[7] * src2[1] 
                + src1[11]* src2[2] + src1[15]* src2[3];
        tm[4] =   src1[0] * src2[4] + src1[4] * src2[5] 
                + src1[8] * src2[6] + src1[12]* src2[7];
        tm[5] =   src1[1] * src2[4] + src1[5] * src2[5] 
                + src1[9] * src2[6] + src1[13]* src2[7];
        tm[6] =   src1[2] * src2[4] + src1[6] * src2[5] 
                + src1[10]* src2[6] + src1[14]* src2[7];
        tm[7] =   src1[3] * src2[4] + src1[7] * src2[5] 
                + src1[11]* src2[6] + src1[15]* src2[7];
        tm[8] =   src1[0] * src2[8] + src1[4] * src2[9] 
                + src1[8] * src2[10]+ src1[12]* src2[11];
        tm[9] =   src1[1] * src2[8] + src1[5] * src2[9] 
                + src1[9] * src2[10]+ src1[13]* src2[11];
        tm[10] =  src1[2] * src2[8] + src1[6] * src2[9] 
                + src1[10]* src2[10]+ src1[14]* src2[11];
        tm[11] =  src1[3] * src2[8] + src1[7] * src2[9] 
                + src1[11]* src2[10]+ src1[15]* src2[11];
        tm[12] =  src1[0] * src2[12]+ src1[4] * src2[13] 
                + src1[8] * src2[14]+ src1[12]* src2[15];
        tm[13] =  src1[1] * src2[12]+ src1[5] * src2[13] 
                + src1[9] * src2[14]+ src1[13]* src2[15];
        tm[14] =  src1[2] * src2[12]+ src1[6] * src2[13] 
                + src1[10]* src2[14]+ src1[14]* src2[15];
        tm[15] =  src1[3] * src2[12]+ src1[7] * src2[13] 
                + src1[11]* src2[14]+ src1[15]* src2[15];

        for (i=0; i<16; i++)
        {
            m[i] = tm[i];
        }
    }
    else 
    {
        fprintf(stderr, "myMultMatrix() error: matrix pointers are null.\n");
    }      
}

/**
 * Translate the 3D object in the scene
 * Input: tx , ty, tz    --- the offset for translation in X, Y, and Z dirctions 
 * Output: m[16]   --- the matrix after the translate
 */
void myTranslate(float m[16], float tx, float ty, float tz)
{
    if (m != NULL )
    {
        m[12] = m[0] * tx + m[4] * ty + m[8] * tz + m[12];
        m[13] = m[1] * tx + m[5] * ty + m[9] * tz + m[13];
        m[14] = m[2] * tx + m[6] * ty + m[10] * tz + m[14];
        m[15] = m[3] * tx + m[7] * ty + m[11] * tz + m[15];
    }
    else 
    {
        fprintf(stderr, "myTranslate() error: matrix pointer is null.\n");
    }
}

/**
 * Scale operation for the object in the scene 
 * Input: sx, sy, sz  --- scaling factors for X, Y and Z directions
 * Output: m[16]  --- the matrix after the scaling
 */
void myScale(float m[16], float sx, float sy, float sz)
{
    int i;
    
    if (m != NULL)
    {
        for (i=0; i<4; i++)
        {
            m[i] = m[i] * sx;
        }

        for (i=4; i<8; i++)
        {
            m[i] = m[i] * sy;
        }

        for (i=8; i<12; i++)
        {
            m[i] = m[i] * sz;
        }
    }
    else
    {
        fprintf(stderr, "myScale() error: matrix pointer is null.\n");
    }

}

/**
 * Rotation operation
 * Input: angle, x, y, z
 * Output: m[16] --- the matrix after the rotation
 */
void myRotate(float m[16], float angle, float x, float y, float z)
{
    float radians, sine, cosine, t; 
    // intermediate variables for matrix calculation
    float vec[3], txy, tyz, txz, sx, sy, sz;  
    // 4x4 matrix for intermediate values
    float tm[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}; 
    
    if (m != NULL)
    {
        radians = angle * M_PI / 180;
        sine = sin(radians);
        cosine = cos(radians);
        t = 1.0 - cosine;
    
        if (normalize(vec, x, y, z) > 0 )
        {
            txy = t * vec[0] * vec[1];
            tyz = t * vec[1] * vec[2];
            txz = t * vec[0] * vec[2];
            sx = sine * vec[0];
            sy = sine * vec[1];
            sz = sine * vec[2];

            tm[0] = t * vec[0] * vec[0] + cosine;
            tm[1] = txy + sz;
            tm[2] = txy - sy;
            tm[4] = txy - sz;
            tm[5] = t * vec[1] * vec[1] + cosine;
            tm[6] = tyz + sx;
            tm[8] = txz + sy;
            tm[9] = tyz - sx;
            tm[10] = t * vec[2] * vec[2] + cosine;
        }
    
        myMultMatrix(m, m, tm);
    }
    else
    {
        fprintf(stderr, "myRotate() error: matrix pointer is null.\n");
    }
}

/**
 * Perspective projection transformation
 * Input: left, right, bottom, top, near, far parameters similar to glFrustum 
 *         function
 * Output: m[16] ---  perspective projection matrix
 */
void myFrustum(float m[16], float left, float right, float bottom, float top, 
               float near, float far)
{
    // temorary 4x4 matrix for intermediate values
    float tm[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    if (m != NULL)
    {
        if (right != left 
            && top != bottom 
            && far != near)
        {
            tm[0] = 2 * near / (right - left);
            tm[5] = 2 * near / (top - bottom);
            tm[8] = (right + left) / (right - left);
            tm[9] = (top + bottom) / (top - bottom);
            tm[10] = (far + near) / (near - far);
            tm[11] = -1;
            tm[14] = 2 * far * near / (near - far);
            tm[15] = 0;

            myMultMatrix(m, m, tm);
        }
        else
        {
            fprintf(stderr, "myFrustum() error: left should be different from "
                            "right. The same is true for top and bottom, far "
                            "and near.\n");
        }
    }
    else
    {
        fprintf(stderr, "myFrustum() error: matrix pointer is null.\n");
    }
}

/**
 * Orthogonal projection transformation
 * Input: left, right, bottom, top, near, far parameters similar to glOrtho 
 *        function
 * Output: m[16]  4x4 projection matrix
 */
void myOrtho(float m[16], float left, float right, float bottom, float top, 
             float near, float far)
{
    // temporary 4x4 matrix for intermediate values
    float tm[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};                
   
    if (m != NULL)
    { 
        if (right != left && top != bottom && far != near)
        {
            tm[0] = 2 / (right - left);
            tm[5] = 2 / (top - bottom);
            tm[10] = 2 / (near - far);
            tm[12] = (right + left) / (right - left);
            tm[13] = (top + bottom) / (top - bottom);
            tm[14] = (far + near) / (far - near);

            myMultMatrix(m, m, tm);
        }
        else
        {
            fprintf(stderr,"myOrtho() error: left should be different from "
                           "right. The same is true for top and bottom, far "
                           "and near.\n");
        } 
    }
    else
    {
        fprintf(stderr, "myOrtho() error: matrix pointer is null.\n");
    }
}

/**
 * Calculate the inverse tanspose matrix of a 4x4 matrix, and take the top 
 * left 3x3 matrix as output matrix.
 * Input: src[16]   4x4 matrix
 * Output: m[9]     3x3 matrix
 */
void myTransposeInvertMatrix(float m[9], float src[16])
{
    float m11, m12, m13, m14, m21, m22, m23, m31, m32, m33;//minors of matrix
    float determinent;                         // determinent value of matrix

    if (m != NULL 
        && src != NULL)
    {
        // Finding minors of src matrix
        m11 = src[5]  * (src[10] * src[15] - src[11] * src[14])
            - src[9]  * (src[6]  * src[15] - src[7]  * src[14])
            + src[13] * (src[6]  * src[11] - src[7]  * src[10]);
        m12 = src[1]  * (src[10] * src[15] - src[11] * src[14])
            - src[9]  * (src[2]  * src[15] - src[3]  * src[14])
            + src[13] * (src[2]  * src[11] - src[3]  * src[10]);
        m13 = src[1]  * (src[6]  * src[15] - src[7]  * src[14])
            - src[5]  * (src[2]  * src[15] - src[3]  * src[14])
            + src[13] * (src[2]  * src[7]  - src[3]  * src[6]);
        m14 = src[1]  * (src[6]  * src[11] - src[7]  * src[10])
            - src[5]  * (src[2]  * src[11] - src[3]  * src[10])
            + src[9]  * (src[2]  * src[7]  - src[3]  * src[6]);
        m21 = src[4]  * (src[10] * src[15] - src[11] * src[14])
            - src[8]  * (src[6]  * src[15] - src[7]  * src[14])
            + src[12] * (src[6]  * src[11] - src[7]  * src[10]);
        m22 = src[0]  * (src[10] * src[15] - src[11] * src[14])
            - src[8]  * (src[2]  * src[15] - src[3]  * src[14])
            + src[12] * (src[2]  * src[11] - src[3]  * src[10]);
        m23 = src[0]  * (src[6]  * src[15] - src[7]  * src[14])
            - src[4]  * (src[2]  * src[15] - src[3]  * src[14])
            + src[12] * (src[2]  * src[7]  - src[3]  * src[6]); 
        m31 = src[4]  * (src[9]  * src[15] - src[11] * src[13])
            - src[8]  * (src[5]  * src[15] - src[7]  * src[13])
            + src[12] * (src[5]  * src[11] - src[7]  * src[9]);
        m32 = src[0]  * (src[9]  * src[15] - src[11] * src[13])
            - src[8]  * (src[1]  * src[15] - src[3]  * src[13])
            + src[12] * (src[1]  * src[11] - src[3]  * src[9]);
        m33 = src[0]  * (src[5]  * src[15] - src[7]  * src[13])
            - src[4]  * (src[1]  * src[15] - src[3]  * src[13])
            + src[12] * (src[1]  * src[7]  - src[3]  * src[5]);

        // calculate the determinent
        determinent = src[0] * m11 - src[4] * m12 + src[8] * m13 - src[12] * m14;
        if (determinent != 0)
        {
            m[0] = m11 / determinent;
            m[1] = -m21 / determinent;
            m[2] = m31 / determinent;
            m[3] = -m12 / determinent;
            m[4] = m22 / determinent;
            m[5] = -m32 / determinent;
            m[6] = m13 / determinent;
            m[7] = -m23 / determinent;
            m[8] = m33 / determinent;
        }
        else
        {
            fprintf(stderr,"myTransposeInvertMatrix() error: no inverse matrix"
                           " exists.\n");
        }
    }
    else
    {
        fprintf(stderr, "myTransposeInvertMatrix() error: matrix pointers are "
                        "null.\n");
    } 
}

/**
 * Calculate the inverse matrix of a 4x4 matrix
 * Input: src[16]   4x4 matrix   
 * Output: m[16]    4x4 matrix
 */
void myInverseMatrix(float m[16], float src[16])
{

    float m11, m12, m13, m14, m21, m22, m23, m24;   // minors of src matrix
    float m31, m32, m33, m34, m41, m42, m43, m44;   // minors of src matrix
    float determinent;

    if (m != NULL
        && src != NULL)
    {
        // Finding minors of src matrix
        m11 = src[5]  * (src[10] * src[15] - src[11] * src[14])
            - src[9]  * (src[6]  * src[15] - src[7]  * src[14])
            + src[13] * (src[6]  * src[11] - src[7]  * src[10]);
        m12 = src[1]  * (src[10] * src[15] - src[11] * src[14])
            - src[9]  * (src[2]  * src[15] - src[3]  * src[14])
            + src[13] * (src[2]  * src[11] - src[3]  * src[10]);
        m13 = src[1]  * (src[6]  * src[15] - src[7]  * src[14])
            - src[5]  * (src[2]  * src[15] - src[3]  * src[14])
            + src[13] * (src[2]  * src[7]  - src[3]  * src[6]);
        m14 = src[1]  * (src[6]  * src[11] - src[7]  * src[10])
            - src[5]  * (src[2]  * src[11] - src[3]  * src[10])
            + src[9]  * (src[2]  * src[7]  - src[3]  * src[6]);
        m21 = src[4]  * (src[10] * src[15] - src[11] * src[14])
            - src[8]  * (src[6]  * src[15] - src[7]  * src[14])
            + src[12] * (src[6]  * src[11] - src[7]  * src[10]);
        m22 = src[0]  * (src[10] * src[15] - src[11] * src[14])
            - src[8]  * (src[2]  * src[15] - src[3]  * src[14])
            + src[12] * (src[2]  * src[11] - src[3]  * src[10]);
        m23 = src[0]  * (src[6]  * src[15] - src[7]  * src[14])
            - src[4]  * (src[2]  * src[15] - src[3]  * src[14])
            + src[12] * (src[2]  * src[7]  - src[3]  * src[6]); 
        m24 = src[0]  * (src[6]  * src[11] - src[7]  * src[10])
            - src[4]  * (src[2]  * src[11] - src[3]  * src[10])
            + src[8]  * (src[2]  * src[7]  - src[3]  * src[6]);
        m31 = src[4]  * (src[9]  * src[15] - src[11] * src[13])
            - src[8]  * (src[5]  * src[15] - src[7]  * src[13])
            + src[12] * (src[5]  * src[11] - src[7]  * src[9]);
        m32 = src[0]  * (src[9]  * src[15] - src[11] * src[13])
            - src[8]  * (src[1]  * src[15] - src[3]  * src[13])
            + src[12] * (src[1]  * src[11] - src[3]  * src[9]);
        m33 = src[0]  * (src[5]  * src[15] - src[7]  * src[13])
            - src[4]  * (src[1]  * src[15] - src[3]  * src[13])
            + src[12] * (src[1]  * src[7]  - src[3]  * src[5]);
        m34 = src[0]  * (src[5]  * src[11] - src[7]  * src[9])
            - src[4]  * (src[1]  * src[11] - src[3]  * src[9])
            + src[8]  * (src[1]  * src[7]  - src[3]  * src[5]);
        m41 = src[4]  * (src[9]  * src[14] - src[10] * src[13])
            - src[8]  * (src[5]  * src[14] - src[6]  * src[13])
            + src[12] * (src[5]  * src[10] - src[6]  * src[9]);
        m42 = src[0]  * (src[9]  * src[14] - src[10] * src[13])
            - src[8]  * (src[1]  * src[14] - src[2]  * src[13])
            + src[12] * (src[1]  * src[10] - src[2]  * src[9]);
        m43 = src[0]  * (src[5]  * src[14] - src[6]  * src[13])
            - src[4]  * (src[1]  * src[14] - src[2]  * src[13])
            + src[12] * (src[1]  * src[6]  - src[2]  * src[5]);
        m44 = src[0]  * (src[5]  * src[10] - src[6]  * src[9])
            - src[4]  * (src[1]  * src[10] - src[2]  * src[9])
            + src[8]  * (src[1]  * src[6]  - src[2]  * src[5]);

        // calculate the determinent
        determinent = src[0] * m11 - src[4] * m12 + src[8] * m13 - src[12] * m14;
        if (determinent != 0)
        {
            m[0] = m11 / determinent;
            m[1] = -m12 / determinent;
            m[2] = m13 / determinent;
            m[3] = -m14 / determinent;
            m[4] = -m21 / determinent;
            m[5] = m22 / determinent;
            m[6] = -m23 / determinent;
            m[7] = m24 / determinent;
            m[8] = m31 / determinent;
            m[9] = -m32 / determinent;
            m[10] = m33 / determinent;
            m[11] = -m34 / determinent;
            m[12] = -m41 / determinent;
            m[13] = m42 / determinent;
            m[14] = -m43 / determinent;
            m[15] = m44 / determinent;
        }
        else
        {
            fprintf(stderr, "myInverseMatrix() error: no inverse matrix "
                            "exists.\n");
        }
    }
    else 
    {
        fprintf(stderr,"myInverseMatrix() error: matrix pointer is null.\n");
    } 
}

/**
 * Calculate the cross product of two input vectors
 * Input: src1[3], src2[3]  --- two input vectors
 * Output: fout[3]  --- output vector     
 */
void crossProduct(float fout[3], float src1[3], float src2[3])
{
    float tmp[3]; 
    if (fout != NULL
        && src1 != NULL
        && src2 != NULL)
    {       
        tmp[0] = src1[1] * src2[2] - src1[2] * src2[1];
        tmp[1] = src1[2] * src2[0] - src1[0] * src2[2];
        tmp[2] = src1[0] * src2[1] - src1[1] * src2[0];
    
        normalize(fout, tmp[0], tmp[1], tmp[2]);
    }
    else
    {
        fprintf(stderr, "crossProduct() error: null input pointers.\n");

    }
}

/** 
 * Calculate the dot product of two vectors
 * Input: src1[3], src2[3]  --- two input vectors
 * return value: dot product of two input vectors
 */
float dotProduct( float src1[3], float src2[3])
{
    float len = 0.0f;  
    if (src1 != NULL
        && src2 != NULL)
    {       
        len = src1[0] * src2[0] + src1[1] * src2[1] + src1[2] * src2[2];
    }
    else
    {
        fprintf(stderr,"dotProduct() error: null input pointers.\n");
    }
    return len;
}

/**
 * Similar to the gluLookAt function.
 * Input: eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ
 * Output: view matrix -- m[16]
 */
void myLookAt(float m[16], float eyeX, float eyeY, float eyeZ, float centerX, 
              float centerY, float centerZ, float upX, float upY, float upZ)
{
    float u[3], v[3], n[3]; // unit vectors for camera view coordinate system
    float n1[3];            // unnormalized forward vector for camera view
    float up[3];            // up vector
    float rotation[16], translate[16]; // rotation matrix and tranlation matrix

    if (m != NULL)
    {
        // calculate the n vector -- forward vector
        n1[0] = eyeX - centerX;
        n1[1] = eyeY - centerY;
        n1[2] = eyeZ - centerZ;
    
        normalize(n, n1[0], n1[1], n1[2]);
        up[0] = upX;
        up[1] = upY;
        up[2] = upZ;
        crossProduct(u, up, n);  // calculate the u vector
        crossProduct(v, n, u);   // calculate the v vector
   
        myIdentity(translate);
        myTranslate(translate, -eyeX, -eyeY, -eyeZ);
        rotation[0] = u[0];
        rotation[1] = v[0];
        rotation[2] = n[0];
        rotation[3] = 0;
        rotation[4] = u[1];
        rotation[5] = v[1];
        rotation[6] = n[1];
        rotation[7] = 0;
        rotation[8] = u[2];
        rotation[9] = v[2];
        rotation[10] = n[2];
        rotation[11] = 0;
        rotation[12] = 0;
        rotation[13] = 0;
        rotation[14] = 0;
        rotation[15] = 1;
      
        myMultMatrix(m, rotation, translate);
    }
    else
    {
        fprintf(stderr, "myLookAt() error: matrix pointer is null.\n");
    }
}
    
