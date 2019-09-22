// Gaslight Games Ltd, (C) 2016-2019. All rights reserved.

#pragma once

#include "Object.h"

#include "ControllerNavigator.generated.h"

// Forward Declarations
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams( FSliderHoverChange, class USlider*, Slider, bool, bHovered );

// Enum of Navigation Directions
UENUM()
enum class ENavDirection : uint8
{
	ND_Up			UMETA( DisplayName = "Navigate Up" ),
	ND_Down			UMETA( DisplayName = "Navigate Down" ),
	ND_Left			UMETA( DisplayName = "Navigate Left" ),
	ND_Right		UMETA( DisplayName = "Navigate Right" )
};

UCLASS()
class CONTROLLERNAVIGATION_API UControllerNavigator : public UObject
{
	GENERATED_BODY()

public:
	UControllerNavigator();

	/**
	* Use this class as a Singleton and thus, returns the current instance.
	* @return UControllerNavigator The singleton active instance.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static UControllerNavigator*		GetInstance();

	/**
	* Cleans up all content within the Singleton.
	* Requests a delete of the current UControllerNavigator instance.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static void							Cleanup();

	/**
	* Attempts to push the passed Widget to the Array of handled and navigable widgets.
	* This doesn't do any kind of checking, if the Widget has already been aded, it will be added again!
	* @param Widget The passed UWidget that we will process and attempt to navigate with Controller/Keyboard input.
	* @return True If supported and successfully added, otherwise False.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static bool							PushWidget( UUserWidget* Widget );

	/**
	* Attempts to pop the last Widget off the Array.
	* @return True If supported and successfully added, otherwise False.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static bool							PopWidget();

	/**
	* Attempts to clear the list of Pushed widgets.
	* @return True if cleared, otherwise false.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static bool							RemoveAllWidgets();

	/**
	* Attempts to Navigate Up.
	* This checks for supported Widgets on screen and if we can navigate in the target direction.
	* Looping means if you navigate up and you are at the top, then it will jump to the bottom most item.
	* Similarly, if you navigate left, and are at the left most item, then it will jump to the right most item.
	* 
	* @param bLoop If True, Navigation will loop.
	* @return True if successfully navigated, otherwise False.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static bool							Up( bool bLoop );

	/**
	* Attempts to Navigate Down.
	* This checks for supported Widgets on screen and if we can navigate in the target direction.
	* Looping means if you navigate up and you are at the top, then it will jump to the bottom most item.
	* Similarly, if you navigate left, and are at the left most item, then it will jump to the right most item.
	*
	* @param bLoop If True, Navigation will loop.
	* @return True if successfully navigated, otherwise False.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static bool							Down( bool bLoop );

	/**
	* Attempts to Navigate Left.
	* This checks for supported Widgets on screen and if we can navigate in the target direction.
	* Looping means if you navigate up and you are at the top, then it will jump to the bottom most item.
	* Similarly, if you navigate left, and are at the left most item, then it will jump to the right most item.
	*
	* @param bLoop If True, Navigation will loop.
	* @return True if successfully navigated, otherwise False.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static bool							Left( bool bLoop );

	/**
	* Attempts to Navigate Right.
	* This checks for supported Widgets on screen and if we can navigate in the target direction.
	* Looping means if you navigate up and you are at the top, then it will jump to the bottom most item.
	* Similarly, if you navigate left, and are at the left most item, then it will jump to the right most item.
	*
	* @param bLoop If True, Navigation will loop.
	* @return True if successfully navigated, otherwise False.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static bool							Right( bool bLoop );

	/**
	* Attempts to select the currently highlighted Widget.
	* @param bForceClick If true, will force a Mouse Click, ignoring if we have any highlighted widget.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static void							Select( bool bForceClick = false );

	/**
	* Checks if the current Widget has a Scrollable (like a ScrollBox) and will then attempt to Scroll
	* that item, in the supplied direction.  This is most useful if an Axis is setup, such as Right Thumbstick
	* then pass values in the range -/+ 0...1.
	* @param ScrollValue The value to attempt to scroll (should be between -1 & +1).
	* @param Multiplier An optional value to allow for faster scrolling.  Defaults to 3x the input.
	* @param bSameInputForSliders If true and currently over a slider, then this input will ignore Scroll boxes.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static void							Scroll( float ScrollValue, float Multiplier = 3.f, bool bSameInputForSliders = false );

	/**
	* Checks if we are currently highlighting a Slider and if so, will attempt to adjust the slider.
	* @param SliderValue The value to attempt to adjust the slider with.
	* @param Multiplier An optional value to adjust for faster/slower sliding. Defaults to 0.1.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static void							Slide( float SlideValue, float Multiplier = .1f );

	/**
	* In order to trigger the over/out events build in to UMG, we move the mouse cursor.  But if you're controlling
	* from a Keyboard/Gamepad, you always want the cursor showing.  We want to let YOU control when the cursor is
	* showing or not, so when we move the cursor, you can check for the Event and if the values match those of where
	* we moved the cursor, you can ignore the movements.
	* Any other movements, the player has "wiggled" the mouse, so you can show the cursor again.
	*
	* @param MouseX The X coordinate from the Mouse event.
	* @param MouseY The Y coordinate from the Mouse event.
	*
	* @return True if the Navigator moved the cursor, otherwise false.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation", meta = ( WorldContext = WorldContextObject ) )
		static bool							NavigatorMovedMouse( float MouseX, float MouseY, UObject* WorldContextObject );

	/**
	* When navigating with this Plugin, by default the Mouse Cursor is hidden.  Use this function to adjust whether
	* this is still the case.
	*
	* @param bHide True if navigation should hide the cursor, false to show the cursor.
	*/
	UFUNCTION( BlueprintCallable, Category = "Controller Navigation" )
		static void							SetNavigationHidesCursor( bool bHide );


	UPROPERTY( BlueprintAssignable, Category = "Controller Navigation" )
		FSliderHoverChange					OnSliderHover;

protected:

	// --------------------------------------------------------------
	// INSTANCE METHODS
	// --------------------------------------------------------------

	/**
	* Loops through the Last item in the Widgets Array and attempts to generate
	* the Array of NavigableObjects.
	*
	* @return True if we have some widgets and we were able to generate our list of navigable ones. Otherwise false.
	*/
	UFUNCTION()
		bool								GenerateNavigableWidgets();

	/**
	* This function receives a Widget and then check if it is a supported Widget type
	* and if so, add it to our our NavigableWidgets Array.
	*
	* @param Widget The UWidget pointer passed from the ForEachWidget() function.
	*/
	UFUNCTION()
		void								PopulateSupportedWidgetsArray( UWidget* Widget );

	/**
	* If a Widget, such as a button, is added to a Panel and this parent Panel is Hidden
	* then all child Widgets (including our button) are hidden.  However, the Visibility
	* value for all child Widgets does not change!
	* This function checks through all parent (and grandparent/great-grandparent etc)
	* ancestors, checking for visibility.
	*
	* @param Widget The UWidget pointer passed to check for ancestral visibility.
	* @return True if all ancestors are visible, otherwise false.
	*/
	UFUNCTION()
		bool								IsAncestorVisible( UWidget* Widget );

	/**
	* Performs the actual attempt to Navigate in the passed direction.
	* @param Direction The ENavDirection we should navigate in.
	* @param bLoop If true, loops to the opposite side.
	* @return True if successfully navigated, otherwise false.
	*/
	UFUNCTION()
		bool								Navigate( ENavDirection Direction, bool bLoop );

	/**
	* Attempts to navigate to the target navigable Widget.
	* @param Widget The Widget we want to attempt to navigate to.
	*/
	UFUNCTION()
		void								NavigateToWidget( UWidget* Widget );

	// --------------------------------------------------------------
	// STATIC PROPERTIES
	// --------------------------------------------------------------

	/** The singleton ControllerNavigator instance. */
	static UControllerNavigator*			ControllerNavigator;

	// --------------------------------------------------------------
	// INSTANCE PROPERTIES
	// --------------------------------------------------------------

	/** The Array of Widgets within the current instance. */
	UPROPERTY()
		TArray<UUserWidget*>				Widgets;

	/** The Array of Navigable Widgets.  I.e. The Widgets that we support navigating to/from. */
	UPROPERTY()
		TArray<UWidget*>					NavigableWidgets;

	/** The current Widget we have navigated to. */
	UPROPERTY()
		UWidget*							CurNavigatedWidget;

	/** The location we have moved the Mouse Cursor. */
	UPROPERTY()
		FVector2D							NavigatorCursorPosition;

	/** On screen cursor values have no rounding, but the calculated pixel/geometry positions of target widgets often will.
	*	To account for this, we allow the cursor to be within a range before we consider it no longer handled by the Navigator.*/
	UPROPERTY()
		float								HideCursorRange;

	/** If we should hide the cursor during Keyboard/Gamepad navigation.  Defaults to true. */
	UPROPERTY()
		bool								bHideCursorDuringNavigation;

private:

};
