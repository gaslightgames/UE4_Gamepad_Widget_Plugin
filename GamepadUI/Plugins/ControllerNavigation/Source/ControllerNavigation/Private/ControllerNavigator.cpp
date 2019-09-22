// Gaslight Games Ltd, (C) 2016-2019. All rights reserved.

#include "ControllerNavigator.h"

#include "ControllerNavigation.h"
#include "Config/ConNavConfig.h"

#include "Widget.h"
#include "WidgetTree.h"
#include "UserWidget.h"

#include "SlateBlueprintLibrary.h"
#include "SlateApplication.h"

#include "Kismet/KismetMathLibrary.h"

// Navigable Supported Widgets
#include "Button.h"
#include "ScrollBox.h"
#include "Slider.h"
//...?

// Static Initialization
UControllerNavigator* UControllerNavigator::ControllerNavigator = nullptr;

UControllerNavigator::UControllerNavigator()
	: CurNavigatedWidget( nullptr )
	, HideCursorRange( 2.f )
	, bHideCursorDuringNavigation( true )
{
	// The editor evaluates the first call to hide the Cursor, down in NavigatorMovedMouse.
	// This additional and Editor only call, fixes that.
#if WITH_EDITOR
	if( FSlateApplication::IsInitialized() )
	{
		if( FSlateApplication::Get().GetPlatformApplication().IsValid() )
		{
			if( FSlateApplication::Get().GetPlatformApplication().Get() != nullptr )
			{
				FSlateApplication::Get().GetPlatformApplication().Get()->Cursor->Show( true );
			}
		}
	}
#endif
}

UControllerNavigator* UControllerNavigator::GetInstance()
{
	if( UControllerNavigator::ControllerNavigator == nullptr )
	{
		//FString MessageText = FString::Printf( TEXT( "Building Navigator..." ) );
		//GEngine->AddOnScreenDebugMessage( -1, 5.f, FColor::Emerald, *MessageText );
		//UE_LOG( ControllerNavigationLog, Warning, TEXT( "%s" ), *MessageText );

		UControllerNavigator::ControllerNavigator = NewObject<UControllerNavigator>();
		UControllerNavigator::ControllerNavigator->AddToRoot();	// Prevent the GC from cleaning up our Instance
	}

	return ControllerNavigator;
}

void UControllerNavigator::Cleanup()
{
	//FString MessageText = FString::Printf( TEXT( "Navigator Cleanup..." ) );
	//GEngine->AddOnScreenDebugMessage( -1, 5.f, FColor::Emerald, *MessageText );
	//UE_LOG( ControllerNavigationLog, Warning, TEXT( "%s" ), *MessageText );

#if WITH_EDITOR
	FSlateApplication::Get().GetPlatformApplication().Get()->Cursor->Show( true );
#endif

	if( UControllerNavigator::ControllerNavigator != nullptr )
	{
		UControllerNavigator::ControllerNavigator->RemoveAllWidgets();
		UControllerNavigator::ControllerNavigator->ConditionalBeginDestroy();
	}
}

bool UControllerNavigator::PushWidget( UUserWidget* Widget )
{
	if( Widget != nullptr )
	{
		// Add will "push" (in all other C++ containers!) the Widget to the END of the Array.
		UControllerNavigator::GetInstance()->Widgets.Add( Widget );// Push( Widget );
		UControllerNavigator::GetInstance()->CurNavigatedWidget = nullptr;
		return true;
	}

	return false;
}

bool UControllerNavigator::PopWidget()
{
	if( UControllerNavigator::GetInstance()->Widgets.Num() > 0 )
	{
		// Needing to guarantee we remove the LAST item from the Widgets.  It is not clear if Pop does this
		// (again, like all other C++ containers!)
		//UControllerNavigator::GetInstance()->Widgets.Pop();
		UControllerNavigator::GetInstance()->Widgets.RemoveAt( UControllerNavigator::GetInstance()->Widgets.Num() - 1 );
		UControllerNavigator::GetInstance()->CurNavigatedWidget = nullptr;
		return true;
	}

	return false;
}

bool UControllerNavigator::RemoveAllWidgets()
{
	if( UControllerNavigator::GetInstance()->Widgets.Num() > 0 )
	{
		UControllerNavigator::GetInstance()->Widgets.Empty();
		UControllerNavigator::GetInstance()->NavigableWidgets.Empty();
		UControllerNavigator::GetInstance()->CurNavigatedWidget = nullptr;
		return true;
	}

	return false;
}

bool UControllerNavigator::Up( bool bLoop )
{
	if( UControllerNavigator::GetInstance()->Widgets.Num() > 0 )
	{
		if( UControllerNavigator::GetInstance()->GenerateNavigableWidgets() )
		{
			return UControllerNavigator::GetInstance()->Navigate( ENavDirection::ND_Up, bLoop );
		}
	}

	return false;
}

bool UControllerNavigator::Down( bool bLoop )
{
	if( UControllerNavigator::GetInstance()->Widgets.Num() > 0 )
	{
		if( UControllerNavigator::GetInstance()->GenerateNavigableWidgets() )
		{
			return UControllerNavigator::GetInstance()->Navigate( ENavDirection::ND_Down, bLoop );
		}
	}

	return false;
}

bool UControllerNavigator::Left( bool bLoop )
{
	if( UControllerNavigator::GetInstance()->Widgets.Num() > 0 )
	{
		if( UControllerNavigator::GetInstance()->GenerateNavigableWidgets() )
		{
			return UControllerNavigator::GetInstance()->Navigate( ENavDirection::ND_Left, bLoop );
		}
	}

	return false;
}

bool UControllerNavigator::Right( bool bLoop )
{
	if( UControllerNavigator::GetInstance()->Widgets.Num() > 0 )
	{
		if( UControllerNavigator::GetInstance()->GenerateNavigableWidgets() )
		{
			return UControllerNavigator::GetInstance()->Navigate( ENavDirection::ND_Right, bLoop );
		}
	}

	return false;
}

void UControllerNavigator::Select( bool bForceClick )
{
	if( UControllerNavigator::GetInstance()->CurNavigatedWidget == nullptr && !bForceClick )
	{
		return;
	}

	if( FSlateApplication::Get().IsInitialized() )
	{
		FSlateApplication& SlateApp = FSlateApplication::Get();
		TSet<FKey> PressedKeys;

		FPointerEvent MouseEvent(
			0,
			SlateApp.GetCursorPos(),
			SlateApp.GetLastCursorPos(),
			PressedKeys,
			EKeys::LeftMouseButton,
			0,
			SlateApp.GetPlatformApplication()->GetModifierKeys()
		);

		TSharedPtr<FGenericWindow> GenWindow;
		SlateApp.ProcessMouseButtonDownEvent( GenWindow, MouseEvent );
		SlateApp.ProcessMouseButtonUpEvent( MouseEvent );
	}
}

void UControllerNavigator::Scroll( float ScrollValue, float Multiplier, bool bSameInputForSliders )
{
	if( UControllerNavigator::GetInstance()->NavigableWidgets.Num() == 0 )
	{
		return;
	}

	if( bSameInputForSliders == true )
	{
		// Check if we are currently on a Slider
		if( UControllerNavigator::GetInstance()->CurNavigatedWidget != nullptr )
		{
			if( UControllerNavigator::GetInstance()->CurNavigatedWidget->IsA( USlider::StaticClass() ) )
			{
				return;
			}
		}
	}

	// Attempt to find a ScrollBox.
	UScrollBox* ScrollBox = nullptr;
	for( int32 i = 0; i < UControllerNavigator::GetInstance()->NavigableWidgets.Num(); ++i )
	{
		if( UControllerNavigator::GetInstance()->NavigableWidgets[i] != nullptr )
		{
			if( UControllerNavigator::GetInstance()->NavigableWidgets[i]->IsA( UScrollBox::StaticClass() ) )
			{
				ScrollBox = Cast<UScrollBox>( UControllerNavigator::GetInstance()->NavigableWidgets[i] );
				break;
			}
		}
	}

	// Exit if we do not.
	if( ScrollBox == nullptr )
	{
		return;
	}

	// Get the Offset, add our value to it, then set the offset.
	ScrollBox->SetScrollOffset( ScrollBox->GetScrollOffset() + ( ScrollValue * Multiplier ) );
}

void UControllerNavigator::Slide( float SlideValue, float Multiplier )
{
	// Check if we are currently on a Slider
	if( UControllerNavigator::GetInstance()->CurNavigatedWidget != nullptr )
	{
		if( UControllerNavigator::GetInstance()->CurNavigatedWidget->IsA( USlider::StaticClass() ) )
		{
			USlider* Slider = Cast<USlider>( UControllerNavigator::GetInstance()->CurNavigatedWidget );
			if( Slider != nullptr )
			{
				float SlideAdjustment = SlideValue * Multiplier;

				// Number of "Steps" the Adjustment contains.
				int32 steps = FMath::RoundToInt( SlideAdjustment / Slider->StepSize );

				// Bring the current Slider value in line with the "stepping"
				int32 curSteps = FMath::RoundToInt( Slider->GetValue() / Slider->StepSize );

				float SliderValue = ( curSteps + steps ) * Slider->StepSize;

				// Ternary's for threshold Max/Min.
				SliderValue = ( SliderValue > 1.f ) ? 1.f : SliderValue;
				SliderValue = ( SliderValue < 0.f ) ? 0.f : SliderValue;

				Slider->SetValue( SliderValue );
				Slider->OnValueChanged.Broadcast( SliderValue );
			}
		}
	}
}

bool UControllerNavigator::NavigatorMovedMouse( float MouseX, float MouseY, UObject* WorldContextObject )
{
	/*
	FString MessageText = FString::Printf( TEXT( "Nav X/Y: %f, %f | Range: %f | Nav(int) X/Y: %d, %d | Mouse X/Y: %f, %f | Mouse(int) X/Y: %d, %d." ),
										   UControllerNavigator::GetInstance()->NavigatorCursorPosition.X,
										   UControllerNavigator::GetInstance()->NavigatorCursorPosition.Y,
										   UControllerNavigator::GetInstance()->HideCursorRange,
										   (int32)UControllerNavigator::GetInstance()->NavigatorCursorPosition.X,
										   (int32)UControllerNavigator::GetInstance()->NavigatorCursorPosition.Y,
										   MouseX, MouseY, (int32)MouseX, (int32)MouseY );
	GEngine->AddOnScreenDebugMessage( -1, 1.f, FColor::Emerald, *MessageText );
	*/
	
	if( FMath::FloorToInt( UControllerNavigator::GetInstance()->NavigatorCursorPosition.X ) == FMath::FloorToInt( MouseX ) &&
		FMath::FloorToInt( UControllerNavigator::GetInstance()->NavigatorCursorPosition.Y ) == FMath::FloorToInt( MouseY ) )
	{
		if( UControllerNavigator::GetInstance()->bHideCursorDuringNavigation )
		{
			// Should hide the Mouse Cursor ONLY if we are in a "Playing" state.
			UWorld* World = GEngine->GetWorldFromContextObjectChecked( WorldContextObject );
			if( World != nullptr )
			{
				if( World->WorldType == EWorldType::Game ||
					World->WorldType == EWorldType::GamePreview ||
					World->WorldType == EWorldType::PIE )
				{
					FSlateApplication::Get().GetPlatformApplication().Get()->Cursor->Show( false );
				}
			}
		}

		return true;
	}

	UControllerNavigator::GetInstance()->CurNavigatedWidget = nullptr;
	
	if( UControllerNavigator::GetInstance()->bHideCursorDuringNavigation )
	{
		FSlateApplication::Get().GetPlatformApplication().Get()->Cursor->Show( true );
	}

	return false;
}

void UControllerNavigator::SetNavigationHidesCursor( bool bHide )
{
	UControllerNavigator::GetInstance()->bHideCursorDuringNavigation = bHide;
}

bool UControllerNavigator::GenerateNavigableWidgets()
{
	UUserWidget* LastWidget = Widgets.Last();
	// IMPORTANT: Passed widget must have bIsFocusable = true (ticked, in editor)
	LastWidget->SetKeyboardFocus();
	TArray<UWidget*> AllWidgets;
	LastWidget->WidgetTree->GetAllWidgets( AllWidgets );
	NavigableWidgets.Empty();

	if( AllWidgets.Num() > 0 )
	{
		for( int32 i = 0; i < AllWidgets.Num(); ++i )
		{
			// Check if our current Widget is a custom Widget, which we may have elements we
			// want to navigate to.
			TArray<UWidget*> ChildWidgets;
			UUserWidget* ChildWidget = Cast<UUserWidget>( AllWidgets[i] );
			if( ChildWidget != nullptr )
			{
				//FString curChildWidgetName = FString::Printf( TEXT( "Cast Child: %s" ), *ChildWidget->GetName() );
				//GEngine->AddOnScreenDebugMessage( -1, 3.f, FColor::Red, *curChildWidgetName );

				// If it *IS* a custom widget, check if it has any children and if so, loop through
				// them, checking for supported widgets.
				//
				// TODO: Perform this loop for any number of depths.
				ChildWidget->WidgetTree->GetAllWidgets( ChildWidgets );
				for( int32 j = 0; j < ChildWidgets.Num(); ++j )
				{
					//FString curChildWidgetName = ChildWidgets[j]->GetName();
					//GEngine->AddOnScreenDebugMessage( -1, 3.f, FColor::Green, *curChildWidgetName );
					PopulateSupportedWidgetsArray( ChildWidgets[j] );
				}
			}

			//FString curWidgetName = AllWidgets[i]->GetName();
			//GEngine->AddOnScreenDebugMessage( -1, 3.f, FColor::Blue, *curWidgetName );
			PopulateSupportedWidgetsArray( AllWidgets[i] );
		}

		// Only ever return true if we have some Widgets to Navigate through.
		if( NavigableWidgets.Num() > 0 )
		{
			return true;
		}
	}
	else
	{
		//FString MessageText = FString::Printf( TEXT( "Got NO Child Widgets!" ) );
		//GEngine->AddOnScreenDebugMessage( -1, 10.f, FColor::Emerald, *MessageText );
		//UE_LOG( ControllerNavigationLog, Warning, TEXT( "%s" ), *MessageText );
	}

	return false;
}

void UControllerNavigator::PopulateSupportedWidgetsArray( UWidget* Widget )
{
	//Widget->GetZOrder
	//FString MessageText = FString::Printf( TEXT( "Button: %s, Y: %f." ), *button->GetLabelText().ToString(), absPos.Y );
	//GEngine->AddOnScreenDebugMessage( -1, 10.f, FColor::Emerald, *MessageText );
	
	// Early exit for Disabled or non-Visible Widgets, including checking all Parent
	// widgets to check for their visibility.
	if( !Widget->bIsEnabled || !Widget->IsVisible() || !IsAncestorVisible( Widget ) )
	{
		return;
	}

	// If the Button does not support Keyboard focus (i.e. "Focusable" is unticked in the editor)
	// then ignore this Button.
	if( Widget->IsA( UButton::StaticClass() ) && !Widget->TakeWidget()->SupportsKeyboardFocus() )
	{
		return;
	}

	if( Widget->IsA( UButton::StaticClass() ) ||
		Widget->IsA( UScrollBox::StaticClass() ) ||
		Widget->IsA( USlider::StaticClass() ) )
	{
		//UButton* button = (UButton*)Widget;
		//FVector2D absPos = button->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
		//FString MessageText = FString::Printf( TEXT( "Button: %s, Y: %f." ), *button->GetLabelText().ToString(), absPos.Y );
		//GEngine->AddOnScreenDebugMessage( -1, 10.f, FColor::Emerald, *MessageText );

		NavigableWidgets.Push( Widget );
	}
}

bool UControllerNavigator::IsAncestorVisible( UWidget* Widget )
{
	bool bHasParent = true;

	// First check for Widgets immediate parent.
	UPanelWidget* parentWidget = Widget->GetParent();
	if( parentWidget != nullptr )
	{
		if( !parentWidget->IsVisible() )
		{
			return false;
		}
	}
	else
	{
		bHasParent = false;
	}

	while( bHasParent )
	{
		UPanelWidget* curParentWidget = parentWidget->GetParent();
		if( curParentWidget != nullptr )
		{
			if( !curParentWidget->IsVisible() )
			{
				return false;
			}
		}
		else
		{
			bHasParent = false;
			break;		// failsafe, in case bHasParent doesn't break our loop.
		}

		// Swap Pointers, so we can move to the next depth.
		parentWidget = curParentWidget;
	}

	return true;
}

bool UControllerNavigator::Navigate( ENavDirection Direction, bool bLoop )
{
	// Early exit if we have no Navigable Widgets.
	if( NavigableWidgets.Num() == 0 )
	{
		return false;
	}

	// If we haven't navigated to a Widget on this Widget - do so.
	if( CurNavigatedWidget == nullptr )
	{		
		// Get the First Widget
		// We need to change this to the most top-left widget, not just the first index!
		// TODO: Change to highlight a better Widget
		// TODO: Update to support NOT highlighting unsupported Widgets and attempt to highlight the next one.
		NavigateToWidget( NavigableWidgets[0] );

		return true;
	}

	// Get the index of the currently highlighted Widget.
	int32 highlightedIndex = -1;
	for( int32 i = 0; i < NavigableWidgets.Num(); ++i )
	{
		if( NavigableWidgets[i] == CurNavigatedWidget )
		{
			highlightedIndex = i;
			break;
		}
	}

	if( highlightedIndex == -1 )
	{
		NavigateToWidget( NavigableWidgets[0] );

		return true;
	}

	// Get the Position of the current Widget
	// Loop through all Widgets.
	// IN the loop, do the switch.
	// Within the switch
	//	- check for the various condition(s)
	//	- i.e. If Up, check if we have any positions higher
	//	- Also check if we're looping and go to the lowest.

	FVector2D CurrentWidgetPos = NavigableWidgets[highlightedIndex]->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
	FVector2D CurrentWidgetSize = NavigableWidgets[highlightedIndex]->GetCachedGeometry().GetLocalSize();
	CurrentWidgetPos.X += CurrentWidgetSize.X * .5f;
	CurrentWidgetPos.Y += CurrentWidgetSize.Y * .5f;

	UWidget* nextWidget = nullptr;

	float NavigationThreshold = 0.f;
	UControllerNavigationConfig* config = GetMutableDefault<UControllerNavigationConfig>();
	if( config != nullptr )
	{
		NavigationThreshold = config->GetNavigationThreshold();
	}

	for( int32 j = 0; j < NavigableWidgets.Num(); ++j )
	{
		// Skip if the current widget is the one we're actively highlighting
		if( CurNavigatedWidget == NavigableWidgets[j] )
		{
			continue;
		}

		// Skip any widgets we "Support" but in different ways, like ScrollBoxes.
		// (Add more items to skip here, if needed).
		if( NavigableWidgets[j]->IsA( UScrollBox::StaticClass() ) )
		{
			continue;
		}

#define NAVIGATOR_MECHANISM 1

#if NAVIGATOR_MECHANISM == 1

		FVector2D CurrentTargetWidgetPos;

		switch( Direction )
		{
			case ENavDirection::ND_Up:
			{
				FVector2D loopWidgetPos = NavigableWidgets[j]->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
				FVector2D loopWidgetSize = NavigableWidgets[j]->GetCachedGeometry().GetLocalSize();
				loopWidgetPos.X += loopWidgetSize.X * .5f;
				loopWidgetPos.Y += loopWidgetSize.Y * .5f;

				// Separately delcare, so that the nextWidgetPos always exists, even if it has 0,0 coordinates.
				FVector2D nextWidgetPos;
				FVector2D nextWidgetSize;
				if( nextWidget != nullptr )
				{
					nextWidgetPos = nextWidget->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
					nextWidgetSize = nextWidget->GetCachedGeometry().GetLocalSize();
					nextWidgetPos.X += nextWidgetSize.X * .5f;
					nextWidgetPos.Y += nextWidgetSize.Y * .5f;
				}
				// First, check the next widget is above our currently highlighted widget.
				if( loopWidgetPos.Y != 0 && loopWidgetPos.Y < CurrentWidgetPos.Y )
				{
					// Guarantees we go "up" if something is above us.
					if( nextWidget == nullptr )
					{
						nextWidget = NavigableWidgets[j];
					}
					else
					{
						// We need the vertical distance between the currently highlighted item and currently known
						// next widget (we're going to move to).
						// And similarly, the vertical distance between the currently highlighted widget and the current
						// loop widget.
						float vertDistCurToNext = CurrentWidgetPos.Y - nextWidgetPos.Y;
						float vertDistCurToLoop = CurrentWidgetPos.Y - loopWidgetPos.Y;

						// Make sure we only move "Up" to the nearest item.
						if( vertDistCurToLoop <= vertDistCurToNext && vertDistCurToLoop > 0 && vertDistCurToNext > 0 )
						{
							// When navigating UP, find the closest widget in the X axis that is above us.
							float horizDistCurToNext = FMath::Abs( CurrentWidgetPos.X - nextWidgetPos.X );
							float horizDistCurToLoop = FMath::Abs( CurrentWidgetPos.X - loopWidgetPos.X );

							if( horizDistCurToLoop - NavigationThreshold < horizDistCurToNext || horizDistCurToLoop + NavigationThreshold < horizDistCurToNext )
							{
								nextWidget = NavigableWidgets[j];
							}
						}
					}
				}

				break;
			}
			case ENavDirection::ND_Down:
			{
				FVector2D loopWidgetPos = NavigableWidgets[j]->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
				FVector2D loopWidgetSize = NavigableWidgets[j]->GetCachedGeometry().GetLocalSize();
				loopWidgetPos.X += loopWidgetSize.X * .5f;
				loopWidgetPos.Y += loopWidgetSize.Y * .5f;

				FVector2D nextWidgetPos;
				FVector2D nextWidgetSize;
				if( nextWidget != nullptr )
				{
					nextWidgetPos = nextWidget->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
					nextWidgetSize = nextWidget->GetCachedGeometry().GetLocalSize();
					nextWidgetPos.X += nextWidgetSize.X * .5f;
					nextWidgetPos.Y += nextWidgetSize.Y * .5f;
				}
				// First, check the next widget is below our currently highlighted widget.
				if( loopWidgetPos.Y != 0 && loopWidgetPos.Y > CurrentWidgetPos.Y )
				{
					// Guarantees we go "down" if something is below us.
					if( nextWidget == nullptr )
					{
						CurrentTargetWidgetPos = loopWidgetPos;
						nextWidget = NavigableWidgets[j];
					}
					else
					{
						// Take the current Widget location minus the TARGET
						//
						// Take the current Widget location minus the LOOP
						//
						// Check their sizes
						// Use only the nearest

						FVector2D CurToTarget = CurrentWidgetPos - CurrentTargetWidgetPos;
						FVector2D CurToLoop = CurrentWidgetPos - loopWidgetPos;

						float distCurToTarget = CurToTarget.Size();
						float distCurToLoop = CurToLoop.Size();

						if( distCurToLoop < distCurToTarget && distCurToLoop > 0 && distCurToTarget > 0 )
						{
							CurrentTargetWidgetPos = loopWidgetPos;
							nextWidget = NavigableWidgets[j];
						}
					}
				}

				break;
			}
			case ENavDirection::ND_Left:
			{
				FVector2D loopWidgetPos = NavigableWidgets[j]->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
				FVector2D loopWidgetSize = NavigableWidgets[j]->GetCachedGeometry().GetLocalSize();
				loopWidgetPos.X += loopWidgetSize.X * .5f;
				loopWidgetPos.Y += loopWidgetSize.Y * .5f;

				FVector2D nextWidgetPos;
				FVector2D nextWidgetSize;
				if( nextWidget != nullptr )
				{
					nextWidgetPos = nextWidget->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
					nextWidgetSize = nextWidget->GetCachedGeometry().GetLocalSize();
					nextWidgetPos.X += nextWidgetSize.X * .5f;
					nextWidgetPos.Y += nextWidgetSize.Y * .5f;
				}

				// First, check the next widget is Left of our currently highlighted widget.
				if( loopWidgetPos.X != 0 && loopWidgetPos.X < CurrentWidgetPos.X )
				{
					// Guarantees we go "Left" if something is Left of us.
					if( nextWidget == nullptr )
					{
						// But still must be within our threshold!
						if( loopWidgetPos.Y < CurrentWidgetPos.Y + NavigationThreshold && loopWidgetPos.Y > CurrentWidgetPos.Y - NavigationThreshold )
						{
							nextWidget = NavigableWidgets[j];
						}
					}
					else
					{
						float horizDistCurToNext = CurrentWidgetPos.X - nextWidgetPos.X;
						float horizDistCurToLoop = CurrentWidgetPos.X - loopWidgetPos.X;

						// Make sure we only move "Left" to the nearest item.
						if( horizDistCurToLoop <= horizDistCurToNext && horizDistCurToLoop > 0 && horizDistCurToNext > 0 )
						{
							if( loopWidgetPos.X != 0 && loopWidgetPos.X < CurrentWidgetPos.X )
							{
								//if( loopWidgetPos.Y < currentWidgetPos.Y + NavigationThreshold && loopWidgetPos.Y > currentWidgetPos.Y - NavigationThreshold )
								//{
								//	nextWidget = NavigableWidgets[j];
								//}

								// When navigating LEFT, find the closest widget in the Y axis that is left of us.
								float vertDistCurToNext = FMath::Abs( CurrentWidgetPos.Y - nextWidgetPos.Y );
								float vertDistCurToLoop = FMath::Abs( CurrentWidgetPos.Y - loopWidgetPos.Y );

								if( vertDistCurToLoop - NavigationThreshold < vertDistCurToNext || vertDistCurToLoop + NavigationThreshold < vertDistCurToNext )
								{
									nextWidget = NavigableWidgets[j];
								}
							}
						}
					}
				}

				break;
			}
			case ENavDirection::ND_Right:
			{
				FVector2D loopWidgetPos = NavigableWidgets[j]->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
				FVector2D loopWidgetSize = NavigableWidgets[j]->GetCachedGeometry().GetLocalSize();
				loopWidgetPos.X += loopWidgetSize.X * .5f;
				loopWidgetPos.Y += loopWidgetSize.Y * .5f;

				FVector2D nextWidgetPos;
				FVector2D nextWidgetSize;
				if( nextWidget != nullptr )
				{
					nextWidgetPos = nextWidget->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
					nextWidgetSize = nextWidget->GetCachedGeometry().GetLocalSize();
					nextWidgetPos.X += nextWidgetSize.X * .5f;
					nextWidgetPos.Y += nextWidgetSize.Y * .5f;
				}

				// First, check the next widget is to the Right of our currently highlighted widget.
				if( loopWidgetPos.X != 0 && loopWidgetPos.X > CurrentWidgetPos.X )
				{
					// Guarantees we go "Right" if something is Right of us.
					if( nextWidget == nullptr )
					{
						// But still must be within our threshold!
						if( loopWidgetPos.Y < CurrentWidgetPos.Y + NavigationThreshold && loopWidgetPos.Y > CurrentWidgetPos.Y - NavigationThreshold )
						{
							nextWidget = NavigableWidgets[j];
						}
					}
					else
					{
						float horizDistNextToCur = nextWidgetPos.X - CurrentWidgetPos.X;
						float horizDistLoopToCur = loopWidgetPos.X - CurrentWidgetPos.X;

						// Make sure we only move "Right" to the nearest item.
						if( horizDistLoopToCur <= horizDistNextToCur && horizDistLoopToCur > 0 && horizDistNextToCur > 0 )
						{
							// First, check the next widget is to the right of our currently highlighted widget.
							if( loopWidgetPos.X != 0 && loopWidgetPos.X > CurrentWidgetPos.X )
							{
								//if( loopWidgetPos.Y < currentWidgetPos.Y + NavigationThreshold && loopWidgetPos.Y > currentWidgetPos.Y - NavigationThreshold )
								//{
								//	nextWidget = NavigableWidgets[j];
								//}

								// When navigating RIGHT, find the closest widget in the Y axis that is right of us.
								float vertDistCurToNext = FMath::Abs( CurrentWidgetPos.Y - nextWidgetPos.Y );
								float vertDistCurToLoop = FMath::Abs( CurrentWidgetPos.Y - loopWidgetPos.Y );

								if( vertDistCurToLoop - NavigationThreshold < vertDistCurToNext || vertDistCurToLoop + NavigationThreshold < vertDistCurToNext )
								{
									nextWidget = NavigableWidgets[j];
								}
							}
						}
					}
				}

				break;
			}
		}
#else

		switch( Direction )
		{
			case ENavDirection::ND_Up:
			{
				FVector2D loopWidgetPos = NavigableWidgets[j]->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
				FVector2D loopWidgetSize = NavigableWidgets[j]->GetCachedGeometry().GetLocalSize();
				loopWidgetPos.X += loopWidgetSize.X * .5f;
				loopWidgetPos.Y += loopWidgetSize.Y * .5f;

				// Separately delcare, so that the nextWidgetPos always exists, even if it has 0,0 coordinates.
				FVector2D nextWidgetPos;
				FVector2D nextWidgetSize;
				if( nextWidget != nullptr )
				{
					nextWidgetPos = nextWidget->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
					nextWidgetSize = nextWidget->GetCachedGeometry().GetLocalSize();
					nextWidgetPos.X += nextWidgetSize.X * .5f;
					nextWidgetPos.Y += nextWidgetSize.Y * .5f;
				}
				// First, check the next widget is above our currently highlighted widget.
				if( loopWidgetPos.Y != 0 && loopWidgetPos.Y < CurrentWidgetPos.Y )
				{
					// Guarantees we go "up" if something is above us.
					if( nextWidget == nullptr )
					{
						nextWidget = NavigableWidgets[j];
					}
					else
					{
						// We need the vertical distance between the currently highlighted item and currently known
						// next widget (we're going to move to).
						// And similarly, the vertical distance between the currently highlighted widget and the current
						// loop widget.
						float vertDistCurToNext = CurrentWidgetPos.Y - nextWidgetPos.Y;
						float vertDistCurToLoop = CurrentWidgetPos.Y - loopWidgetPos.Y;

						// Make sure we only move "Up" to the nearest item.
						if( vertDistCurToLoop <= vertDistCurToNext && vertDistCurToLoop > 0 && vertDistCurToNext > 0 )
						{
							// When navigating UP, find the closest widget in the X axis that is above us.
							float horizDistCurToNext = FMath::Abs( CurrentWidgetPos.X - nextWidgetPos.X );
							float horizDistCurToLoop = FMath::Abs( CurrentWidgetPos.X - loopWidgetPos.X );
							
							if( horizDistCurToLoop - NavigationThreshold < horizDistCurToNext || horizDistCurToLoop + NavigationThreshold < horizDistCurToNext )
							{
								nextWidget = NavigableWidgets[j];
							}
						}
					}
				}

				break;
			}
			case ENavDirection::ND_Down:
			{
				FVector2D loopWidgetPos = NavigableWidgets[j]->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
				FVector2D loopWidgetSize = NavigableWidgets[j]->GetCachedGeometry().GetLocalSize();
				loopWidgetPos.X += loopWidgetSize.X * .5f;
				loopWidgetPos.Y += loopWidgetSize.Y * .5f;

				FVector2D nextWidgetPos;
				FVector2D nextWidgetSize;
				if( nextWidget != nullptr )
				{
					nextWidgetPos = nextWidget->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
					nextWidgetSize = nextWidget->GetCachedGeometry().GetLocalSize();
					nextWidgetPos.X += nextWidgetSize.X * .5f;
					nextWidgetPos.Y += nextWidgetSize.Y * .5f;
				}
				// First, check the next widget is below our currently highlighted widget.
				if( loopWidgetPos.Y != 0 && loopWidgetPos.Y > CurrentWidgetPos.Y )
				{
					// Guarantees we go "down" if something is below us.
					if( nextWidget == nullptr )
					{
						nextWidget = NavigableWidgets[j];
					}
					else
					{
						// We need the vertical distance between the currently known next widget (we're going to move to)
						// and the currently highlighted item.
						// And similarly, the vertical distance between the current loop widget
						// and the currently highlighted widget.
						// (The opposite to "Up", in the code above).
						float vertDistNextToCur = nextWidgetPos.Y - CurrentWidgetPos.Y;
						float vertDistLoopToCur = loopWidgetPos.Y - CurrentWidgetPos.Y;

						if( vertDistLoopToCur <= vertDistNextToCur && vertDistLoopToCur > 0 && vertDistNextToCur > 0 )
						{
							// When navigating DOWN, find the closest widget in the X axis that is below us.
							float horizDistCurToNext = FMath::Abs( CurrentWidgetPos.X - nextWidgetPos.X );
							float horizDistCurToLoop = FMath::Abs( CurrentWidgetPos.X - loopWidgetPos.X );

							if( horizDistCurToLoop - NavigationThreshold < horizDistCurToNext || horizDistCurToLoop + NavigationThreshold < horizDistCurToNext )
							{
								nextWidget = NavigableWidgets[j];
							}
						}
					}
				}

				break;
			}
			case ENavDirection::ND_Left:
			{
				FVector2D loopWidgetPos = NavigableWidgets[j]->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
				FVector2D loopWidgetSize = NavigableWidgets[j]->GetCachedGeometry().GetLocalSize();
				loopWidgetPos.X += loopWidgetSize.X * .5f;
				loopWidgetPos.Y += loopWidgetSize.Y * .5f;

				FVector2D nextWidgetPos;
				FVector2D nextWidgetSize;
				if( nextWidget != nullptr )
				{
					nextWidgetPos = nextWidget->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
					nextWidgetSize = nextWidget->GetCachedGeometry().GetLocalSize();
					nextWidgetPos.X += nextWidgetSize.X * .5f;
					nextWidgetPos.Y += nextWidgetSize.Y * .5f;
				}

				// First, check the next widget is Left of our currently highlighted widget.
				if( loopWidgetPos.X != 0 && loopWidgetPos.X < CurrentWidgetPos.X )
				{
					// Guarantees we go "Left" if something is Left of us.
					if( nextWidget == nullptr )
					{
						// But still must be within our threshold!
						if( loopWidgetPos.Y < CurrentWidgetPos.Y + NavigationThreshold && loopWidgetPos.Y > CurrentWidgetPos.Y - NavigationThreshold )
						{
							nextWidget = NavigableWidgets[j];
						}
					}
					else
					{
						float horizDistCurToNext = CurrentWidgetPos.X - nextWidgetPos.X;
						float horizDistCurToLoop = CurrentWidgetPos.X - loopWidgetPos.X;

						// Make sure we only move "Left" to the nearest item.
						if( horizDistCurToLoop <= horizDistCurToNext && horizDistCurToLoop > 0 && horizDistCurToNext > 0 )
						{
							if( loopWidgetPos.X != 0 && loopWidgetPos.X < CurrentWidgetPos.X )
							{
								//if( loopWidgetPos.Y < currentWidgetPos.Y + NavigationThreshold && loopWidgetPos.Y > currentWidgetPos.Y - NavigationThreshold )
								//{
								//	nextWidget = NavigableWidgets[j];
								//}

								// When navigating LEFT, find the closest widget in the Y axis that is left of us.
								float vertDistCurToNext = FMath::Abs( CurrentWidgetPos.Y - nextWidgetPos.Y );
								float vertDistCurToLoop = FMath::Abs( CurrentWidgetPos.Y - loopWidgetPos.Y );

								if( vertDistCurToLoop - NavigationThreshold < vertDistCurToNext || vertDistCurToLoop + NavigationThreshold < vertDistCurToNext )
								{
									nextWidget = NavigableWidgets[j];
								}
							}
						}
					}
				}

				break;
			}
			case ENavDirection::ND_Right:
			{
				FVector2D loopWidgetPos = NavigableWidgets[j]->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
				FVector2D loopWidgetSize = NavigableWidgets[j]->GetCachedGeometry().GetLocalSize();
				loopWidgetPos.X += loopWidgetSize.X * .5f;
				loopWidgetPos.Y += loopWidgetSize.Y * .5f;

				FVector2D nextWidgetPos;
				FVector2D nextWidgetSize;
				if( nextWidget != nullptr )
				{
					nextWidgetPos = nextWidget->GetCachedGeometry().LocalToAbsolute( FVector2D( 0, 0 ) );
					nextWidgetSize = nextWidget->GetCachedGeometry().GetLocalSize();
					nextWidgetPos.X += nextWidgetSize.X * .5f;
					nextWidgetPos.Y += nextWidgetSize.Y * .5f;
				}

				// First, check the next widget is to the Right of our currently highlighted widget.
				if( loopWidgetPos.X != 0 && loopWidgetPos.X > CurrentWidgetPos.X )
				{
					// Guarantees we go "Right" if something is Right of us.
					if( nextWidget == nullptr )
					{
						// But still must be within our threshold!
						if( loopWidgetPos.Y < CurrentWidgetPos.Y + NavigationThreshold && loopWidgetPos.Y > CurrentWidgetPos.Y - NavigationThreshold )
						{
							nextWidget = NavigableWidgets[j];
						}
					}
					else
					{
						float horizDistNextToCur = nextWidgetPos.X - CurrentWidgetPos.X;
						float horizDistLoopToCur = loopWidgetPos.X - CurrentWidgetPos.X;

						// Make sure we only move "Right" to the nearest item.
						if( horizDistLoopToCur <= horizDistNextToCur && horizDistLoopToCur > 0 && horizDistNextToCur > 0 )
						{
							// First, check the next widget is to the right of our currently highlighted widget.
							if( loopWidgetPos.X != 0 && loopWidgetPos.X > CurrentWidgetPos.X )
							{
								//if( loopWidgetPos.Y < currentWidgetPos.Y + NavigationThreshold && loopWidgetPos.Y > currentWidgetPos.Y - NavigationThreshold )
								//{
								//	nextWidget = NavigableWidgets[j];
								//}

								// When navigating RIGHT, find the closest widget in the Y axis that is right of us.
								float vertDistCurToNext = FMath::Abs( CurrentWidgetPos.Y - nextWidgetPos.Y );
								float vertDistCurToLoop = FMath::Abs( CurrentWidgetPos.Y - loopWidgetPos.Y );

								if( vertDistCurToLoop - NavigationThreshold < vertDistCurToNext || vertDistCurToLoop + NavigationThreshold < vertDistCurToNext )
								{
									nextWidget = NavigableWidgets[j];
								}
							}
						}
					}
				}

				break;
			}
		}

#endif
	}

	if( nextWidget == nullptr )
	{
		return false;
	}
	else
	{
		NavigateToWidget( nextWidget );
		return true;
	}
}

void UControllerNavigator::NavigateToWidget( UWidget* Widget )
{
	//FString MessageText = FString::Printf( TEXT( "Navigating to Widget: %s." ), *Widget->GetName() );
	//GEngine->AddOnScreenDebugMessage( -1, 10.f, FColor::Emerald, *MessageText );

	// This can be empty, invalid or out of date - but it's the only way we can get the Widget's Geometry.
	// Also, if the widget is bIsVolatile, then this will never get generated!!!
	FGeometry widgetGeom = Widget->GetCachedGeometry();
	FGeometry viewportAdjustedGeom;

	UGameViewportClient* ViewClient = GEngine->GameViewport;
	if( ViewClient != nullptr )
	{
		FViewport* Viewport = ViewClient->Viewport;
		if( Viewport != nullptr )
		{
			FVector2D pixelPos;
			FVector2D viewportPos;
			FVector2D LocalCoords;
			LocalCoords.X = Widget->GetCachedGeometry().GetLocalSize().X * .5f;
			LocalCoords.Y = Widget->GetCachedGeometry().GetLocalSize().Y * .5f;
			
			//USlateBlueprintLibrary::LocalToViewport( Widget, widgetGeom, FVector2D( 0.f, 0.f ), pixelPos, viewportPos );
			USlateBlueprintLibrary::LocalToViewport( Widget, widgetGeom, LocalCoords, pixelPos, viewportPos );

			float CursorOffsetX = 0.f;
			float CursorOffsetY = 0.f;
			UControllerNavigationConfig* config = GetMutableDefault<UControllerNavigationConfig>();
			if( config != nullptr )
			{
				CursorOffsetX = config->GetCursorOffsetX();
				CursorOffsetY = config->GetCursorOffsetY();
			}

			NavigatorCursorPosition.X = pixelPos.X + CursorOffsetX;
			NavigatorCursorPosition.Y = pixelPos.Y + CursorOffsetY;

			if( NavigatorCursorPosition.X > 0 && NavigatorCursorPosition.Y > 0 )
			{
				UWidget* OldNavigatedWidget = CurNavigatedWidget;
				CurNavigatedWidget = Widget;
				Viewport->SetMouse( (int32)NavigatorCursorPosition.X, (int32)NavigatorCursorPosition.Y );

				// MJ: 23/03/2018
				// This process allows us to handle "hover" when used via this Plugin, but not generic
				// mouse movements.
				// If this becomes an issue, due to inconsistencies in UX, then consider creating an
				// extended SSlider and USlider that implement an actual Un/Hover event system.

				// Unhover the previous Widget - if it was a Slider.
				if( OldNavigatedWidget != nullptr )
				{
					if( OldNavigatedWidget->IsA( USlider::StaticClass() ) )
					{
						if( OnSliderHover.IsBound() )
						{
							OnSliderHover.Broadcast( Cast<USlider>( OldNavigatedWidget ), false );
						}
					}
				}

				// If the new Widget is a Slider, "Hover" it
				if( CurNavigatedWidget != nullptr )
				{
					if( CurNavigatedWidget->IsA( USlider::StaticClass() ) )
					{
						if( OnSliderHover.IsBound() )
						{
							OnSliderHover.Broadcast( Cast<USlider>( CurNavigatedWidget ), true );
						}
					}
				}
			}
		}
	}
}
