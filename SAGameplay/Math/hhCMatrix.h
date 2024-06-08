#pragma once

namespace Hedgehog::Math
{

    class CMatrix;
    class CMatrix44;

    class CMatrix : public Eigen::Affine3f
    {
    public:
        using Eigen::Affine3f::Affine3f;
        CMatrix& operator=(const CVector& vector)
        {
            Eigen::Affine3f* This = this;
            *This = Eigen::Translation3f(vector);
            return *this;
        }


        // Eigen assignments are templated, so as a hack solution, force assignments to use base class assignment.
        template <typename T>
        CMatrix& operator=(const T& other)
        {
            Eigen::Affine3f* This = this;
            *This = other;
            return *this;
        }

        CMatrix(const CVector& vector)
        {
            *(Eigen::Affine3f*)m_matrix.data() = Eigen::Translation3f(vector);
        }
        CMatrix(const Eigen::AlignedVector3<float>& vector)
        {
            *(Eigen::Affine3f*)m_matrix.data() = Eigen::Translation3f(vector);
        }

        CMatrix(const Eigen::Affine3f& affine)
        {
            *this = affine;
        }

        static void CreateFromAxis(CMatrix* This, CVector* xAxis, CVector* yAxis, CVector* zAxis);

        static CMatrix CreateFromAxis(const CVector& xAxis, const CVector& yAxis, const CVector& zAxis);

        static CMatrix* Transpose(void* This, void* Result);

        CVector GetVectorFromRow(int row) const;

        CVector GetVectorFromColumn(int column) const;

        static CVector* TransformNormal(CMatrix44* This, CVector* result, CVector* vec);

        Eigen::Matrix3f GetRotationMatrix();
    };

    class CMatrix44 : public Eigen::Matrix4f
    {
        using Eigen::Matrix4f::Matrix4f;
        //using Eigen::Matrix4f::operator=;

    public:
        // Eigen assignments are templated, so as a hack solution, force assignments to use base class assignment.
        template <typename T>
        void operator=(const T& other)
        {
            Eigen::Matrix4f* This = (Eigen::Matrix4f*)this;
            *This = other;
        }

        static void CreateFromAxis(CMatrix44* This, const CVector* xAxis, const CVector* yAxis, const CVector* zAxis);

        static CMatrix44 CreateFromAxis(const CVector& xAxis, const CVector& yAxis, const CVector& zAxis);

        static CMatrix44 CreateFromAxis(CVector* xAxis, CVector* yAxis, CVector* zAxis);

        void SetFromAxis(CVector* xAxis, CVector* yAxis, CVector* zAxis);

        float GetFloatFromMatrix(int row, int column);

        CVector GetVectorFromRow(int row) const;

        CVector GetVectorFromColumn(int row) const;

        CVector TransformVector(const CVector& vector) const;

        void RotateYaw(float angle);

        void RotatePitch(float angle);
    };
}