#include "TriangleDemo.h"
#include "Game.h"
#include "GameException.h"
#include "MatrixHelper.h"
#include "Camera.h"
#include "Utility.h"
#include "D3DCompiler.h"

namespace Rendering
{
    RTTI_DEFINITIONS(TriangleDemo)

        TriangleDemo::TriangleDemo(Game& game, Camera& camera)
        : DrawableGameComponent(game, camera),
        mEffect(nullptr), mTechnique(nullptr), mPass(nullptr), mWvpVariable(nullptr),
        mInputLayout(nullptr), mWorldMatrix(MatrixHelper::Identity), mVertexBuffer(nullptr), mAngle(0.0f)
    {
    }

    TriangleDemo::~TriangleDemo()
    {
        ReleaseObject(mWvpVariable);
        ReleaseObject(mPass);
        ReleaseObject(mTechnique);
        ReleaseObject(mEffect);
        ReleaseObject(mInputLayout);
        ReleaseObject(mVertexBuffer);
    }

    void TriangleDemo::Initialize()
    {
        SetCurrentDirectory(Utility::ExecutableDirectory().c_str());

        // Compile the shader
        UINT shaderFlags = 0;

#if defined( DEBUG ) || defined( _DEBUG )
        shaderFlags |= D3DCOMPILE_DEBUG;
        shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

        ID3D10Blob* compiledShader = nullptr;
        ID3D10Blob* errorMessages = nullptr;

        // 1. load the effect file (vertex and pixel shader)
        HRESULT hr = D3DCompileFromFile(L"Content\\Effects\\BasicEffect.fx", nullptr,
            nullptr, nullptr, "fx_5_0", shaderFlags, 0, &compiledShader, &errorMessages);
        if (FAILED(hr))
        {
            const char* errorMessage = (errorMessages != nullptr ?
                (char*)errorMessages->GetBufferPointer() : "D3DX11CompileFromFile() failed");
            GameException ex(errorMessage, hr);
            ReleaseObject(errorMessages);
            throw ex;
        }

        // Create an effect object from the compiled shader
        hr = D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(),
            compiledShader->GetBufferSize(), 0, mGame->Direct3DDevice(), &mEffect);
        if (FAILED(hr))
        {
            throw GameException("D3DX11CreateEffectFromMemory() failed.", hr);
        }

        ReleaseObject(compiledShader);

        // Look up the technique, pass, and WVP variable from the effect
        mTechnique = mEffect->GetTechniqueByName("main11");
        if (mTechnique == nullptr)
        {
            throw GameException("ID3DX11Effect::GetTechniqueByName() could not find the specified technique.", hr);
        }

        mPass = mTechnique->GetPassByName("p0");
        if (mPass == nullptr)
        {
            throw GameException("ID3DX11EffectTechnique::GetPassByName() could not find the specified pass.", hr);
        }

        ID3DX11EffectVariable* variable = mEffect->GetVariableByName("WorldViewProjection");
        if (variable == nullptr)
        {
            throw GameException("ID3DX11Effect::GetVariableByName() could not find the specified variable.", hr);
        }

        mWvpVariable = variable->AsMatrix();
        if (mWvpVariable->IsValid() == false)
        {
            throw GameException("Invalid effect variable cast.");
        }

        // Create the input layout
        D3DX11_PASS_DESC passDesc;
        mPass->GetDesc(&passDesc);

        // 2. create the vertex layout
        D3D11_INPUT_ELEMENT_DESC inputElementDescriptions[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,
            D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
            D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        if (FAILED(hr = mGame->Direct3DDevice()->CreateInputLayout(inputElementDescriptions,
            ARRAYSIZE(inputElementDescriptions), passDesc.pIAInputSignature,
            passDesc.IAInputSignatureSize, &mInputLayout)))
        {
            throw GameException("ID3D11Device::CreateInputLayout() failed.", hr);
        }

        // 3. Create the vertex buffer - SINGLE 2D STAR with INNER SQUARE
        BasicEffectVertex vertices[] =
        {
            // === STAR POINTS (4 triangles - 12 vertices) ===

            // Triangle 1 (top point) - Red/Orange/Yellow
            BasicEffectVertex(XMFLOAT4(0.0f, 0.8f, 0.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)),   // top (red)
            BasicEffectVertex(XMFLOAT4(0.25f, 0.25f, 0.0f, 1.0f), XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f)), // right-upper (orange)
            BasicEffectVertex(XMFLOAT4(-0.25f, 0.25f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)), // left-upper (yellow)

            // Triangle 2 (right point) - Green/Cyan/Orange
            BasicEffectVertex(XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)),   // right (green)
            BasicEffectVertex(XMFLOAT4(0.25f, -0.25f, 0.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)), // right-lower (cyan)
            BasicEffectVertex(XMFLOAT4(0.25f, 0.25f, 0.0f, 1.0f), XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f)), // right-upper (orange)

            // Triangle 3 (bottom point) - Blue/Purple/Cyan
            BasicEffectVertex(XMFLOAT4(0.0f, -0.8f, 0.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)),  // bottom (blue)
            BasicEffectVertex(XMFLOAT4(-0.25f, -0.25f, 0.0f, 1.0f), XMFLOAT4(0.5f, 0.0f, 1.0f, 1.0f)), // left-lower (purple)
            BasicEffectVertex(XMFLOAT4(0.25f, -0.25f, 0.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)), // right-lower (cyan)

            // Triangle 4 (left point) - Magenta/Yellow/Purple
            BasicEffectVertex(XMFLOAT4(-0.8f, 0.0f, 0.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)),  // left (magenta)
            BasicEffectVertex(XMFLOAT4(-0.25f, 0.25f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)), // left-upper (yellow)
            BasicEffectVertex(XMFLOAT4(-0.25f, -0.25f, 0.0f, 1.0f), XMFLOAT4(0.5f, 0.0f, 1.0f, 1.0f)), // left-lower (purple)

            // === INNER SQUARE (2 triangles - 6 vertices) ===
            // Square fits perfectly at the junction points (±0.25, ±0.25)

            // Triangle 5 (square - bottom-left half)
            BasicEffectVertex(XMFLOAT4(-0.25f, -0.25f, 0.0f, 1.0f), XMFLOAT4(0.5f, 0.0f, 1.0f, 1.0f)), // bottom-left (purple)
            BasicEffectVertex(XMFLOAT4(0.25f, -0.25f, 0.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)),  // bottom-right (cyan)
            BasicEffectVertex(XMFLOAT4(-0.25f, 0.25f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)), // top-left (yellow)

            // Triangle 6 (square - top-right half)
            BasicEffectVertex(XMFLOAT4(0.25f, -0.25f, 0.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)),  // bottom-right (cyan)
            BasicEffectVertex(XMFLOAT4(0.25f, 0.25f, 0.0f, 1.0f), XMFLOAT4(1.0f, 0.5f, 0.0f, 1.0f)),   // top-right (orange)
            BasicEffectVertex(XMFLOAT4(-0.25f, 0.25f, 0.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f))  // top-left (yellow)
        };

        D3D11_BUFFER_DESC vertexBufferDesc;
        ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
        vertexBufferDesc.ByteWidth = sizeof(BasicEffectVertex) * ARRAYSIZE(vertices); // 18 vertices (4 triangles + 2 triangles)
        vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertexSubResourceData;
        ZeroMemory(&vertexSubResourceData, sizeof(vertexSubResourceData));
        vertexSubResourceData.pSysMem = vertices;

        if (FAILED(mGame->Direct3DDevice()->CreateBuffer(&vertexBufferDesc,
            &vertexSubResourceData, &mVertexBuffer)))
        {
            throw GameException("ID3D11Device::CreateBuffer() failed.");
        }
    }

    void TriangleDemo::Update(const GameTime& gameTime)
    {
        // Add rotation speed (adjust as needed)
        float rotationSpeed = 0.5f;

        // Calculate rotation angle based on elapsed time
        mAngle += XM_PI * rotationSpeed * static_cast<float>(gameTime.ElapsedGameTime());


        XMMATRIX rotationX = XMMatrixRotationX(mAngle * 0.5f);
        XMMATRIX rotationY = XMMatrixRotationY(mAngle);
        XMMATRIX rotationZ = XMMatrixRotationZ(mAngle * 0.3f);
        XMStoreFloat4x4(&mWorldMatrix, rotationX * rotationY * rotationZ);
    }

    void TriangleDemo::Draw(const GameTime& gameTime)
    {
        ID3D11DeviceContext* direct3DDeviceContext = mGame->Direct3DDeviceContext();

        direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        direct3DDeviceContext->IASetInputLayout(mInputLayout);

        UINT stride = sizeof(BasicEffectVertex);
        UINT offset = 0;
        direct3DDeviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset);

        XMMATRIX worldMatrix = XMLoadFloat4x4(&mWorldMatrix);
        XMMATRIX wvp = worldMatrix * mCamera->ViewMatrix() * mCamera->ProjectionMatrix();
        mWvpVariable->SetMatrix(reinterpret_cast<const float*>(&wvp));
        mPass->Apply(0, direct3DDeviceContext);

        // Draw all 18 vertices (6 triangles total)
        direct3DDeviceContext->Draw(18, 0);
    }
}