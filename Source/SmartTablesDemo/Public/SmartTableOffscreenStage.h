#pragma once

#include "Containers/Set.h"
#include "GameFramework/Actor.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Logging/LogMacros.h"
#include "Math/IntPoint.h"
#include "Math/Vector2D.h"
#include "Templates/SharedPointer.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectPtr.h"

#include "SmartTableOffscreenStage.generated.h"

class FWidgetPath;
class FWidgetRenderer;
class FSlateVirtualUserHandle;
class SVirtualWindow;
class SWidget;
class UNativeWidgetHost;
class UTextureRenderTarget2D;
class UUserWidget;
struct FSlateBrush;

SMARTTABLESDEMO_API DECLARE_LOG_CATEGORY_EXTERN( LogSmartTableOffscreen, Log, All );

UCLASS()
class SMARTTABLESDEMO_API ASmartTableOffscreenStage : public AActor
{
    GENERATED_BODY()

public:
    ASmartTableOffscreenStage();

    virtual void Tick( float DeltaSeconds ) override;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Offscreen", meta = ( ToolTip = "" ) )
    TSubclassOf< UUserWidget > PageClass;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Offscreen", meta = ( ClampMin = "256", ToolTip = "" ) )
    FIntPoint LayoutSize = FIntPoint( 1600, 900 );

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Offscreen", meta = ( ClampMin = "0.25", ClampMax = "4.0", ToolTip = "" ) )
    float RenderScale = 1.173f;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Offscreen" )
    bool bLogEveryPress = true;

    UPROPERTY( Transient )
    TObjectPtr< class UPanelWidget > DisplayHost;

    void SetRenderScale( float NewScale );

    float GetRenderScale() const
    {
        return RenderScale;
    }

    FIntPoint GetLayoutSize() const
    {
        return LayoutSize;
    }

    UUserWidget * GetPage() const
    {
        return Page;
    }

    int32 GetVirtualUserIndex() const;

    FVector2D GetPointerPosition() const
    {
        return PointerPosition;
    }

    FString DescribeCaptor() const;

    FString DescribePointerTarget() const;

    FString DescribeMapping() const;

    bool RouteWheel( float Delta );

    bool PressOnPage( FVector2D UV, const FKey & Button, bool bDown );

    bool MoveOnPage( FVector2D UV );

    FVector2D GetLastPressUV() const
    {
        return LastPressUV;
    }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay( const EEndPlayReason::Type Reason ) override;

private:
    void BuildPage();
    void TearDownPage();

    void ShowRenderTarget();

    FReply HandleDisplayPointer( const FGeometry & Geometry, const FPointerEvent & MouseEvent );

    FVector2D PixelSize() const;

    void MatchWindowToDraw();

    void BuildRenderTarget();

    void DrawPage( float DeltaSeconds );

    void RoutePointer();

    FWidgetPath PathUnderPointer() const;

    TOptional< FVector2D > CursorOverImage() const;

    TOptional< FVector2D > CursorAsTheGameSeesIt() const;

    void ClearPointer();

    UPROPERTY( Transient )
    TObjectPtr< UUserWidget > Page;

    UPROPERTY( Transient )
    TObjectPtr< UTextureRenderTarget2D > RenderTarget;

    TSharedPtr< SVirtualWindow > SlateWindow;

    TSharedPtr< class SDPIScaler > PageScaler;

    TWeakPtr< SWidget > DisplaySurface;

    TSharedPtr< SWidget > ViewportDisplay;

    UPROPERTY( Transient )
    TObjectPtr< UNativeWidgetHost > HostedDisplay;

    TSharedPtr< FSlateBrush > DisplayBrush;

    TSharedPtr< FSlateVirtualUserHandle > VirtualUser;

    TSharedPtr< class IInputProcessor > WheelForwarder;

    FWidgetRenderer * WidgetRenderer = nullptr;

    FVector2D LastPressUV         = FVector2D( 0.5, 0.5 );
    FVector2D PointerPosition     = FVector2D::ZeroVector;
    FVector2D LastPointerPosition = FVector2D::ZeroVector;

    TSet< FKey > HeldButtons;
};
