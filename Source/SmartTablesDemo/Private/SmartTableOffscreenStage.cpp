#include "SmartTableOffscreenStage.h"

#include "Blueprint/UserWidget.h"
#include "Components/NativeWidgetHost.h"
#include "Components/PanelWidget.h"
#include "Containers/ArrayView.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "GameFramework/PlayerController.h"
#include "Input/HittestGrid.h"
#include "Layout/ArrangedWidget.h"
#include "Layout/WidgetPath.h"
#include "Logging/StructuredLog.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/AssertionMacros.h"
#include "RenderDeferredCleanup.h"
#include "Slate/WidgetRenderer.h"
#include "Styling/SlateBrush.h"
#include "UnrealClient.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SViewport.h"
#include "Widgets/Layout/SDPIScaler.h"
#include "Widgets/SVirtualWindow.h"

DEFINE_LOG_CATEGORY( LogSmartTableOffscreen );

namespace
{

    constexpr int32 DisplayZOrder = 100;

    constexpr int32 VirtualPointerIndex = 0;

    FString Describe( FString Captor )
    {
        return Captor.IsEmpty() ? FString( TEXT( "none" ) ) : Captor;
    }

    class FOffscreenWheelForwarder : public IInputProcessor
    {
    public:
        explicit FOffscreenWheelForwarder( ASmartTableOffscreenStage & InStage )
            : Stage( &InStage )
        {
        }

        virtual void Tick( const float, FSlateApplication &, TSharedRef< ICursor > ) override
        {
        }

        virtual bool HandleMouseWheelOrGestureEvent( FSlateApplication &, const FPointerEvent & WheelEvent, const FPointerEvent * ) override
        {
            ASmartTableOffscreenStage * Live = Stage.Get();

            return Live && Live->RouteWheel( WheelEvent.GetWheelDelta() );
        }

    private:
        TWeakObjectPtr< ASmartTableOffscreenStage > Stage;
    };
}

ASmartTableOffscreenStage::ASmartTableOffscreenStage()
{
    PrimaryActorTick.bCanEverTick = true;

    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void ASmartTableOffscreenStage::BeginPlay()
{
    Super::BeginPlay();

    BuildPage();
}

void ASmartTableOffscreenStage::EndPlay( const EEndPlayReason::Type Reason )
{
    TearDownPage();

    Super::EndPlay( Reason );
}

void ASmartTableOffscreenStage::BuildPage()
{
    if ( !PageClass )
    {
        UE_LOGFMT( LogSmartTableOffscreen, Warning, "{Actor} has no PageClass, so there is nothing to draw. Set it on the placed actor - the demo hub Blueprint is the one worth pointing it at.", GetName() );

        return;
    }

    if ( !FSlateApplication::IsInitialized() )
    {
        UE_LOGFMT( LogSmartTableOffscreen, Warning, "{Actor} drew nothing. Slate is not up, which is a null RHI or a dedicated server.", GetName() );

        return;
    }

    checkf( GetWorld(), TEXT( "%s is running BeginPlay with no world" ), *GetName() );

    APlayerController * Controller = GetWorld()->GetFirstPlayerController();
    if ( !Controller )
    {
        UE_LOGFMT( LogSmartTableOffscreen, Warning, "{Actor} found no player controller to own the page, so nothing is drawn. Give the level a PlayerController - the default game mode's will do.", GetName() );

        return;
    }

    Page = CreateWidget< UUserWidget >( Controller, PageClass );
    if ( !Page )
    {
        UE_LOGFMT( LogSmartTableOffscreen, Warning, "{Actor} could not create a widget of class {Class}. Point PageClass at a concrete (non-Abstract) UserWidget.", GetName(), PageClass->GetName() );

        return;
    }

    BuildRenderTarget();

    SlateWindow = SNew( SVirtualWindow ).Size( PixelSize() );

    SlateWindow->SetVisibility( EVisibility::Visible );

    SlateWindow->SetContent(
        SAssignNew( PageScaler, SDPIScaler )
            .DPIScale( RenderScale )
            [
                Page->TakeWidget()
            ] );

    FSlateApplication::Get().RegisterVirtualWindow( SlateWindow.ToSharedRef() );

    WidgetRenderer = new FWidgetRenderer( false );

    VirtualUser = FSlateApplication::Get().FindOrCreateVirtualUser( 0 );

    ShowRenderTarget();

    WheelForwarder = MakeShared< FOffscreenWheelForwarder >( *this );
    FSlateApplication::Get().RegisterInputPreProcessor( WheelForwarder );

    Controller->bShowMouseCursor = true;
    Controller->SetInputMode( FInputModeGameAndUI().SetLockMouseToViewportBehavior( EMouseLockMode::DoNotLock ) );

    UE_LOGFMT( LogSmartTableOffscreen, Log, "[offscreen] {Actor} up: {Page} laid out {LayoutW}x{LayoutH}, drawn {PixelW}x{PixelH} - scale {Scale}. Virtual user {User}.", GetName(), Page->GetClass()->GetName(), LayoutSize.X, LayoutSize.Y, RenderTarget->SizeX, RenderTarget->SizeY, RenderScale, GetVirtualUserIndex() );
}

void ASmartTableOffscreenStage::ShowRenderTarget()
{
    DisplayBrush = MakeShared< FSlateBrush >();
    DisplayBrush->SetResourceObject( RenderTarget );

    DisplayBrush->ImageSize = FVector2f( LayoutSize.X, LayoutSize.Y );

    const FPointerEventHandler Swallow = FPointerEventHandler::CreateUObject( this, &ASmartTableOffscreenStage::HandleDisplayPointer );

    const TSharedRef< SWidget > Surface =
        SNew( SBorder )
            .BorderImage( nullptr )
            .Padding( 0.0f )

            .Visibility( EVisibility::Visible )
            .OnMouseButtonDown( Swallow )
            .OnMouseButtonUp( Swallow )
            [
                SNew( SImage )
                    .Image( DisplayBrush.Get() )
                    .Visibility( EVisibility::HitTestInvisible )
            ];

    DisplaySurface = Surface;

    if ( DisplayHost )
    {

        HostedDisplay = NewObject< UNativeWidgetHost >( this );
        HostedDisplay->SetContent( Surface );

        DisplayHost->AddChild( HostedDisplay );

        return;
    }

    ViewportDisplay =
        SNew( SBox )
            .HAlign( HAlign_Left )
            .VAlign( VAlign_Top )
            .WidthOverride( static_cast< float >( LayoutSize.X ) )
            .HeightOverride( static_cast< float >( LayoutSize.Y ) )
            [
                Surface
            ];

    checkf( GEngine && GEngine->GameViewport, TEXT( "%s has a player controller and no game viewport" ), *GetName() );
    GEngine->GameViewport->AddViewportWidgetContent( ViewportDisplay.ToSharedRef(), DisplayZOrder );
}

FVector2D ASmartTableOffscreenStage::PixelSize() const
{
    return FVector2D( LayoutSize.X, LayoutSize.Y ) * RenderScale;
}

void ASmartTableOffscreenStage::MatchWindowToDraw()
{
    if ( SlateWindow.IsValid() )
    {
        SlateWindow->Resize( PixelSize() );
    }

    if ( PageScaler.IsValid() )
    {
        PageScaler->SetDPIScale( RenderScale );
    }
}

void ASmartTableOffscreenStage::BuildRenderTarget()
{

    RenderTarget = FWidgetRenderer::CreateTargetFor( PixelSize(), TF_Bilinear, false );
}

void ASmartTableOffscreenStage::TearDownPage()
{
    if ( FSlateApplication::IsInitialized() )
    {
        if ( WheelForwarder.IsValid() )
        {
            FSlateApplication::Get().UnregisterInputPreProcessor( WheelForwarder );
        }

        if ( GEngine && GEngine->GameViewport && ViewportDisplay.IsValid() )
        {
            GEngine->GameViewport->RemoveViewportWidgetContent( ViewportDisplay.ToSharedRef() );
        }

        if ( SlateWindow.IsValid() )
        {
            FSlateApplication::Get().UnregisterVirtualWindow( SlateWindow.ToSharedRef() );
        }
    }

    if ( HostedDisplay )
    {
        HostedDisplay->RemoveFromParent();
        HostedDisplay = nullptr;
    }

    WheelForwarder.Reset();
    DisplayHost = nullptr;
    DisplaySurface.Reset();
    ViewportDisplay.Reset();
    DisplayBrush.Reset();
    PageScaler.Reset();
    SlateWindow.Reset();
    VirtualUser.Reset();

    if ( WidgetRenderer )
    {

        BeginCleanup( WidgetRenderer );
        WidgetRenderer = nullptr;
    }

    RenderTarget = nullptr;
    Page         = nullptr;
}

void ASmartTableOffscreenStage::SetRenderScale( float NewScale )
{
    RenderScale = FMath::Clamp( NewScale, 0.25f, 4.0f );

    if ( !RenderTarget )
    {
        return;
    }

    BuildRenderTarget();
    MatchWindowToDraw();

    if ( DisplayBrush.IsValid() )
    {
        DisplayBrush->SetResourceObject( RenderTarget );
    }

    UE_LOGFMT( LogSmartTableOffscreen, Log, "[offscreen] {Actor} redraws {PixelW}x{PixelH} - scale {Scale}. The page keeps its {LayoutW}x{LayoutH} layout.", GetName(), RenderTarget->SizeX, RenderTarget->SizeY, RenderScale, LayoutSize.X, LayoutSize.Y );
}

void ASmartTableOffscreenStage::Tick( float DeltaSeconds )
{
    Super::Tick( DeltaSeconds );

    if ( !SlateWindow.IsValid() || !WidgetRenderer || !RenderTarget )
    {
        return;
    }

    DrawPage( DeltaSeconds );
    RoutePointer();
}

void ASmartTableOffscreenStage::DrawPage( float DeltaSeconds )
{

    WidgetRenderer->DrawWindow( RenderTarget, SlateWindow->GetHittestGrid(), SlateWindow.ToSharedRef(), 1.0f, FVector2D( RenderTarget->SizeX, RenderTarget->SizeY ), DeltaSeconds );
}

TOptional< FVector2D > ASmartTableOffscreenStage::CursorOverImage() const
{
    const TSharedPtr< SWidget > Surface = DisplaySurface.Pin();
    if ( !Surface.IsValid() )
    {
        return TOptional< FVector2D >();
    }

    const FGeometry & Drawn = Surface->GetTickSpaceGeometry();
    const FVector2D Size    = FVector2D( Drawn.GetLocalSize() );
    if ( Size.X <= 0.0f || Size.Y <= 0.0f )
    {

        return TOptional< FVector2D >();
    }

    const TOptional< FVector2D > Cursor = CursorAsTheGameSeesIt();
    if ( !Cursor.IsSet() )
    {
        return TOptional< FVector2D >();
    }

    const FVector2D Local = FVector2D( Drawn.AbsoluteToLocal( FVector2f( Cursor.GetValue() ) ) );
    const FVector2D UV    = Local / Size;

    if ( UV.X < 0.0f || UV.X > 1.0f || UV.Y < 0.0f || UV.Y > 1.0f )
    {
        return TOptional< FVector2D >();
    }

    return UV;
}

FWidgetPath ASmartTableOffscreenStage::PathUnderPointer() const
{
    if ( !SlateWindow.IsValid() )
    {
        return FWidgetPath();
    }

    TArray< FWidgetAndPointer > Bubble = SlateWindow->GetHittestGrid().GetBubblePath( PointerPosition, 0.0f, false, INDEX_NONE );

    const FVirtualPointerPosition Virtual( PointerPosition, LastPointerPosition );
    for ( FWidgetAndPointer & Arranged : Bubble )
    {
        Arranged.SetPointerPosition( Virtual );
    }

    return FWidgetPath( MakeArrayView( Bubble ) );
}

TOptional< FVector2D > ASmartTableOffscreenStage::CursorAsTheGameSeesIt() const
{
    const UWorld * World                = GetWorld();
    const APlayerController * Controller = World ? World->GetFirstPlayerController() : nullptr;
    const UGameViewportClient * Client  = World ? World->GetGameViewport() : nullptr;

    float PixelX = 0.0f;
    float PixelY = 0.0f;
    if ( !Controller || !Client || !Client->Viewport || !Controller->GetMousePosition( PixelX, PixelY ) )
    {
        return TOptional< FVector2D >();
    }

    const TSharedPtr< SViewport > Widget = Client->GetGameViewportWidget();
    const FIntPoint Pixels               = Client->Viewport->GetSizeXY();
    if ( !Widget.IsValid() || Pixels.X <= 0 || Pixels.Y <= 0 )
    {
        return TOptional< FVector2D >();
    }

    const FGeometry & Geometry = Widget->GetTickSpaceGeometry();
    const FVector2D LocalSize  = FVector2D( Geometry.GetLocalSize() );
    const FVector2D Local( PixelX * LocalSize.X / Pixels.X, PixelY * LocalSize.Y / Pixels.Y );

    return FVector2D( Geometry.LocalToAbsolute( FVector2f( Local ) ) );
}

void ASmartTableOffscreenStage::ClearPointer()
{
    LastPointerPosition = PointerPosition;
    PointerPosition     = FVector2D( -1.0, -1.0 );

    const FPointerEvent Move( GetVirtualUserIndex(), VirtualPointerIndex, PointerPosition, LastPointerPosition, HeldButtons, FKey(), 0.0f, FModifierKeysState() );

    FSlateApplication::Get().RoutePointerMoveEvent( FWidgetPath(), Move, false );
}

void ASmartTableOffscreenStage::RoutePointer()
{
    const TOptional< FVector2D > UV = CursorOverImage();
    if ( !UV.IsSet() )
    {
        ClearPointer();
        return;
    }

    FSlateApplication & Slate = FSlateApplication::Get();

    LastPointerPosition = PointerPosition;

    PointerPosition = FVector2D( UV->X * LayoutSize.X, UV->Y * LayoutSize.Y ) * RenderScale;

    const FPointerEvent Move( GetVirtualUserIndex(), VirtualPointerIndex, PointerPosition, LastPointerPosition, HeldButtons, FKey(), 0.0f, FModifierKeysState() );

    if ( bLogEveryPress && !HeldButtons.IsEmpty() )
    {
        if ( const TSharedPtr< FSlateUser > User = Slate.GetUser( GetVirtualUserIndex() ) )
        {
            const FWidgetPath Routed = User->GetCaptorPath( VirtualPointerIndex, FWeakWidgetPath::EInterruptedPathHandling::ReturnInvalid, &Move );

            if ( Routed.IsValid() )
            {
                const FArrangedWidget & Leaf = Routed.Widgets.Last();
                const FVector2D LeafAt       = FVector2D( Leaf.Geometry.LocalToAbsolute( FVector2f::ZeroVector ) );
                const FVector2D InLeaf       = FVector2D( Leaf.Geometry.AbsoluteToLocal( FVector2f( PointerPosition ) ) );

                UE_LOGFMT( LogSmartTableOffscreen, Log, "[offscreen] ROUTED to '{Type}' local={W}x{H} scale={Scale} topLeft={X},{Y} in={IX},{IY}", Leaf.Widget->GetTypeAsString(), Leaf.Geometry.GetLocalSize().X, Leaf.Geometry.GetLocalSize().Y, Leaf.Geometry.Scale, LeafAt.X, LeafAt.Y, InLeaf.X, InLeaf.Y );
            }
        }
    }

    const bool bMoveHandled = Slate.RoutePointerMoveEvent( PathUnderPointer(), Move, false );

    if ( bLogEveryPress && !HeldButtons.IsEmpty() && PointerPosition != LastPointerPosition )
    {
        UE_LOGFMT( LogSmartTableOffscreen, Log, "[offscreen] MOVE to {X},{Y} handled={Handled} captor='{Captor}'", FMath::RoundToInt( PointerPosition.X ), FMath::RoundToInt( PointerPosition.Y ), bMoveHandled, Describe( DescribeCaptor() ) );
    }

    const TSet< FKey > & RealButtons = Slate.GetPressedMouseButtons();

    for ( const FKey & Button : { EKeys::LeftMouseButton, EKeys::RightMouseButton } )
    {
        const bool bDownNow = RealButtons.Contains( Button );
        if ( bDownNow == HeldButtons.Contains( Button ) )
        {
            continue;
        }

        if ( bDownNow )
        {
            HeldButtons.Add( Button );

            const FWidgetPath Under = PathUnderPointer();
            const FString Target    = DescribePointerTarget();

            const bool bMenusBefore = Slate.AnyMenusVisible();

            const FPointerEvent Down( GetVirtualUserIndex(), VirtualPointerIndex, PointerPosition, LastPointerPosition, HeldButtons, Button, 0.0f, FModifierKeysState() );
            const bool bHandled = Slate.RoutePointerDownEvent( Under, Down ).IsEventHandled();

            if ( bLogEveryPress )
            {
                UE_LOGFMT( LogSmartTableOffscreen, Log, "[offscreen] PRESS {Button} at {X},{Y} over '{Target}' handled={Handled} menus={Before}->{After} captor='{Captor}'", Button.GetFName(), FMath::RoundToInt( PointerPosition.X ), FMath::RoundToInt( PointerPosition.Y ), Target, bHandled, bMenusBefore, Slate.AnyMenusVisible(), Describe( DescribeCaptor() ) );
            }
        }
        else
        {
            HeldButtons.Remove( Button );

            const FString Target = DescribePointerTarget();

            const FPointerEvent Up( GetVirtualUserIndex(), VirtualPointerIndex, PointerPosition, LastPointerPosition, HeldButtons, Button, 0.0f, FModifierKeysState() );
            const bool bHandled = Slate.RoutePointerUpEvent( PathUnderPointer(), Up ).IsEventHandled();

            if ( bLogEveryPress )
            {
                UE_LOGFMT( LogSmartTableOffscreen, Log, "[offscreen] RELEASE {Button} at {X},{Y} over '{Target}' handled={Handled} menus={Menus} captor='{Captor}'", Button.GetFName(), FMath::RoundToInt( PointerPosition.X ), FMath::RoundToInt( PointerPosition.Y ), Target, bHandled, Slate.AnyMenusVisible(), Describe( DescribeCaptor() ) );
            }
        }
    }
}

FReply ASmartTableOffscreenStage::HandleDisplayPointer( const FGeometry &, const FPointerEvent & MouseEvent )
{

    FReply Taken = FReply::Handled();

    if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || MouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
    {
        if ( const TSharedPtr< SViewport > GameViewport = FSlateApplication::Get().GetGameViewport() )
        {
            Taken.SetUserFocus( GameViewport.ToSharedRef(), EFocusCause::Mouse );
        }
    }

    return Taken;
}

bool ASmartTableOffscreenStage::MoveOnPage( FVector2D UV )
{
    const TSharedPtr< SWidget > Surface = DisplaySurface.Pin();
    if ( !Surface.IsValid() )
    {
        return false;
    }

    const FGeometry & Drawn = Surface->GetTickSpaceGeometry();
    const FVector2D Size    = FVector2D( Drawn.GetLocalSize() );
    if ( Size.X <= 0.0f || Size.Y <= 0.0f )
    {
        return false;
    }

    LastPressUV = UV;

    FSlateApplication::Get().SetCursorPos( FVector2D( Drawn.LocalToAbsolute( FVector2f( UV * Size ) ) ) );

    return true;
}

bool ASmartTableOffscreenStage::PressOnPage( FVector2D UV, const FKey & Button, bool bDown )
{
    const TSharedPtr< SWidget > Surface = DisplaySurface.Pin();
    if ( !Surface.IsValid() )
    {
        return false;
    }

    const FGeometry & Drawn = Surface->GetTickSpaceGeometry();
    const FVector2D Size    = FVector2D( Drawn.GetLocalSize() );
    if ( Size.X <= 0.0f || Size.Y <= 0.0f )
    {
        return false;
    }

    LastPressUV = UV;

    FSlateApplication & Slate = FSlateApplication::Get();

    const FVector2D Screen = FVector2D( Drawn.LocalToAbsolute( FVector2f( UV * Size ) ) );
    Slate.SetCursorPos( Screen );

    TSet< FKey > Pressed;
    if ( bDown )
    {
        Pressed.Add( Button );
    }

    const FPointerEvent Event( FSlateApplication::CursorUserIndex, FSlateApplicationBase::CursorPointerIndex, Screen, Screen, Pressed, Button, 0.0f, FModifierKeysState() );

    if ( bDown )
    {
        Slate.ProcessMouseButtonDownEvent( nullptr, Event );
    }
    else
    {
        Slate.ProcessMouseButtonUpEvent( Event );
    }

    UE_LOGFMT( LogSmartTableOffscreen, Log, "[offscreen] real {Verb} {Button} at screen {X},{Y} (uv {U},{V}). The virtual one follows on the next tick.", bDown ? TEXT( "PRESS" ) : TEXT( "RELEASE" ), Button.GetFName(), FMath::RoundToInt( Screen.X ), FMath::RoundToInt( Screen.Y ), UV.X, UV.Y );

    return true;
}

bool ASmartTableOffscreenStage::RouteWheel( float Delta )
{
    if ( !SlateWindow.IsValid() || !CursorOverImage().IsSet() )
    {
        return false;
    }

    const FPointerEvent Wheel( GetVirtualUserIndex(), VirtualPointerIndex, PointerPosition, PointerPosition, HeldButtons, EKeys::MouseWheelAxis, Delta, FModifierKeysState() );

    return FSlateApplication::Get().RouteMouseWheelOrGestureEvent( PathUnderPointer(), Wheel, nullptr ).IsEventHandled();
}

int32 ASmartTableOffscreenStage::GetVirtualUserIndex() const
{
    return VirtualUser.IsValid() ? VirtualUser->GetUserIndex() : INDEX_NONE;
}

FString ASmartTableOffscreenStage::DescribeCaptor() const
{
    if ( !VirtualUser.IsValid() || !FSlateApplication::IsInitialized() )
    {
        return FString();
    }

    const TSharedPtr< const FSlateUser > User = FSlateApplication::Get().GetUser( GetVirtualUserIndex() );
    if ( !User.IsValid() )
    {
        return FString();
    }

    const TSharedPtr< SWidget > Captor = User->GetPointerCaptor( VirtualPointerIndex );

    return Captor.IsValid() ? Captor->GetTypeAsString() : FString();
}

FString ASmartTableOffscreenStage::DescribeMapping() const
{
    const TSharedPtr< SWidget > Surface = DisplaySurface.Pin();
    if ( !Surface.IsValid() )
    {
        return TEXT( "no drawn surface" );
    }

    const FGeometry & Drawn      = Surface->GetTickSpaceGeometry();
    const FVector2D Size         = FVector2D( Drawn.GetLocalSize() );
    const FVector2D TopLeft      = FVector2D( Drawn.LocalToAbsolute( FVector2f::ZeroVector ) );
    const FVector2D Cursor       = FVector2D( FSlateApplication::Get().GetCursorPos() );
    const TOptional< FVector2D > Game = CursorAsTheGameSeesIt();
    const TOptional< FVector2D > UV = CursorOverImage();

    const FString GameSees = Game.IsSet() ? FString::Printf( TEXT( "%.0f,%.0f" ), Game->X, Game->Y ) : FString( TEXT( "none" ) );

    const FString Fraction = UV.IsSet() ? FString::Printf( TEXT( "%.4f,%.4f" ), UV->X, UV->Y ) : FString( TEXT( "off-page" ) );

    FString Captured( TEXT( "captor none" ) );

    if ( const TSharedPtr< const FSlateUser > User = FSlateApplication::Get().GetUser( GetVirtualUserIndex() ) )
    {
        if ( const TSharedPtr< SWidget > Captor = User->GetPointerCaptor( VirtualPointerIndex ) )
        {
            const FGeometry & Held    = Captor->GetTickSpaceGeometry();
            const FVector2D HeldSize  = FVector2D( Held.GetLocalSize() );
            const FVector2D HeldAt    = FVector2D( Held.LocalToAbsolute( FVector2f::ZeroVector ) );
            const FVector2D InCaptor  = FVector2D( Held.AbsoluteToLocal( FVector2f( PointerPosition ) ) );

            const FGeometry & Painted   = Captor->GetPaintSpaceGeometry();
            const FVector2D PaintedSize = FVector2D( Painted.GetLocalSize() );
            const FVector2D PaintedAt   = FVector2D( Painted.LocalToAbsolute( FVector2f::ZeroVector ) );
            const FVector2D InPainted   = FVector2D( Painted.AbsoluteToLocal( FVector2f( PointerPosition ) ) );

            FSlateUser & Live = const_cast< FSlateUser & >( *User );
            const FWidgetPath CaptorPath = Live.GetCaptorPath( VirtualPointerIndex, FWeakWidgetPath::EInterruptedPathHandling::ReturnInvalid );

            FString Value;
            if ( Captor->GetType() == FName( TEXT( "SSlider" ) ) )
            {
                Value = FString::Printf( TEXT( " value=%.4f" ), StaticCastSharedPtr< SSlider >( Captor )->GetValue() );
            }

            Captured = FString::Printf( TEXT( "captor '%s'%s path=%s depth=%d | TICK local=%.0fx%.0f scale=%.3f topLeft=%.0f,%.0f in=%.1f,%.1f | PAINT local=%.0fx%.0f scale=%.3f topLeft=%.0f,%.0f in=%.1f,%.1f" ),
                                        *Captor->GetTypeAsString(), *Value,
                                        CaptorPath.IsValid() ? TEXT( "whole" ) : TEXT( "TRUNCATED" ), CaptorPath.Widgets.Num(),
                                        HeldSize.X, HeldSize.Y, Held.Scale, HeldAt.X, HeldAt.Y, InCaptor.X, InCaptor.Y,
                                        PaintedSize.X, PaintedSize.Y, Painted.Scale, PaintedAt.X, PaintedAt.Y, InPainted.X, InPainted.Y );
        }
    }

    return FString::Printf( TEXT( "surface local=%.0fx%.0f scale=%.3f topLeft=%.0f,%.0f | cursor=%.0f,%.0f game=%s uv=%s | page=%.0f,%.0f of %dx%d px, laid out %dx%d | %s" ),
                            Size.X, Size.Y, Drawn.Scale, TopLeft.X, TopLeft.Y,
                            Cursor.X, Cursor.Y, *GameSees, *Fraction,
                            PointerPosition.X, PointerPosition.Y,
                            RenderTarget ? RenderTarget->SizeX : 0, RenderTarget ? RenderTarget->SizeY : 0,
                            LayoutSize.X, LayoutSize.Y, *Captured );
}

FString ASmartTableOffscreenStage::DescribePointerTarget() const
{
    const FWidgetPath Under = PathUnderPointer();
    if ( !Under.IsValid() )
    {
        return TEXT( "nothing" );
    }

    return Under.GetLastWidget()->GetTypeAsString();
}
