/**
 * ScreenUi
 * A toolkit for building character based user interfaces on small displays.
 * Copyright (c) 2012 Jason von Nieda <jason@vonnieda.org>
 *
 * This file is part of ScreenUi.
 *
 * ScreenUi is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ScreenUi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ScreenUi.  If not, see <http://www.gnu.org/licenses/>. 
 */

#ifndef __ScreenUi_h__
#define __ScreenUi_h__

#include <stdint.h>

//#define SCREENUI_DEBUG 1

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

class Screen;

class Component {
  public:
    Component() { x_ = y_ = width_ = height_ = 0; }
    // Set the location on screen for this component. x and y are zero based,
    // absolute character positions.
    virtual void setLocation(int8_t x, int8_t y) { x_ = x; y_ = y;}
    // Set the width and height this component. Most components will either
    // require a width and height to be specified during creation or will
    // adopt sane defaults based on their input data.
    void setSize(uint8_t width, uint8_t height) { width_ = width; height_ = height; }
    int8_t x() { return x_; }
    int8_t y() { return y_; }
    uint8_t width() { return width_; }
    uint8_t height() { return height_; }
    // Returns true if the component is willing to accept focus from the focus
    // subsystem. For a component to receive input events it must be willing
    // to accept focus.
    virtual bool acceptsFocus() { return false; }
    // The first step in the component update cycle. This is called by Screen
    // during it's update cycle to allow each component to reset or set up
    // any data that needs to be modified from the last update cycle.
    virtual void update(Screen *screen) {}
    // Called if the component has focus and is selected. x and y are delta
    // since the last event.
    // Returns true if the component wishes to remain selected. Returns false
    // to give up selection.
    virtual bool handleInputEvent(int x, int y, bool selected, bool cancelled) { return false; }
    // The final step in the component update cycle. Called by Screen to allow
    // the component to draw itself on screen. It shoud generally draw itself
    // at it's location and should not overflow it's size.
    // Currently components are responsible for drawing their own focus
    // indicator. This may change in the future.
    // Component::paint() sets dirty to false and should be called by every
    // subclass's paint method.
    virtual void paint(Screen *screen);
    // Returns true if the component is a container for other components. This
    // is a shortcut so that we don't have to do RTTI when iterating over a
    // list of components looking for containers.
    virtual bool isContainer() { return false; }
    // Returns true if the Component is marked dirty and needs to be painted
    // on the next update.
    virtual bool dirty();
    // Sets dirty to true for this Component, causing it to be painted during
    // the next update.
    // TODO refactor the two below into setDirty()
    virtual void repaint() { dirty_ = true; }
    virtual void clearDirty() { dirty_ = false; }
    #ifdef SCREENUI_DEBUG
    virtual char *description() { return "Component"; }
    #endif
	protected:
		int8_t x_, y_;
		uint8_t width_, height_;
		bool dirty_;
};

// A Component that contains other Components. Users should not generally
// create instances of this class.
class Container : public Component {
  public:
    Container();
    ~Container();
    virtual void add(Component *component, int8_t x, int8_t y);
    virtual void update(Screen *screen);
    // Paints any dirty child components.
    virtual void paint(Screen *screen);
    virtual bool isContainer() { return true; }
    // Returns true if any child components are dirty.
    virtual bool dirty();
    // Sets dirty to true for all child components, causing them to be repainted
    // during the next update.
    virtual void repaint();
    #ifdef SCREENUI_DEBUG
    virtual char *description() { return "Container"; }
    #endif
    virtual bool contains(Component *component);
  protected:
    Component *nextFocusHolder(Component *focusHolder, bool reverse);
    Component *nextFocusHolder(Component *focusHolder, bool reverse, bool *focusHolderFound);

    Component **components_;
    uint8_t componentsLength_;
    uint8_t componentCount_;
};

// The main entry point into the ScreenUi system. A Screen instance represents
// a full screen of data on the display, including modifiable Components and
// provides methods for input and output.
// The user should create instances of Screen to manage a user interface and
// add() Components to the screen.
// When a Screen is ready to be displayed, the user should call update() in
// a loop. After each call to update(), Components can be queried for their
// data.
class Screen : public Container {
  public:
    Screen(uint8_t width, uint8_t height);
    // Should be called regularly by the main program to update the Screen
    // and process input. After each call to update(), each Component
    // will have processed any input it received and will have updated it
    // data.
    virtual void update();
    // Returns the current focus holder for the Screen. The focus holder is
    // the component that input events will be sent to.
    Component *focusHolder() { return focusHolder_; }
    // Sets the current focus holder. This can be used to set the default
    // button on a screen before it is displayed, for instance.
    void setFocusHolder(Component *focusHolder) { focusHolder_ = focusHolder; }
    void setCursorLocation(uint8_t x, uint8_t y) { cursorX_ = x; cursorY_ = y; }

    #ifdef SCREENUI_DEBUG
    virtual char *description() { return "Screen"; }
    #endif
    
    // The following methods must be overridden by the user to provide
    // hardware support
    
    // Get any changes in the input since the last call to update();
    virtual void getInputDeltas(int *x, int *y, bool *selected, bool *cancelled);
    // Clear the screen
    virtual void clear();
    virtual void createCustomChar(uint8_t slot, uint8_t *data);
    virtual void draw(uint8_t x, uint8_t y, const char *text);
    virtual void draw(uint8_t x, uint8_t y, uint8_t customChar);
    virtual void setCursorVisible(bool visible);
    virtual void setBlink(bool blink);
    virtual void moveCursor(uint8_t x, uint8_t y);

  private:
    bool cleared_;
    Component *focusHolder_;
    bool focusHolderSelected_;
    bool annoyingBugWorkedAround_;
    uint8_t cursorX_, cursorY_;
};

// A Component that displays static text at a specific position. 
class Label : public Component {
  public:
    Label(const char *text);
    virtual void setText(const char *text);
    virtual void paint(Screen *screen);
    #ifdef SCREENUI_DEBUG
    virtual char *description() { return "Label"; }
    #endif
  protected:
    char* text_;
    bool captured_;
    uint8_t dirtyWidth_;
};

// A Component that can receive focus and select events. If the Button has
// focus when the user presses the select button the Button's pressed() property
// is set indicating that the button was selected.
// Button is a subclass of Label and thus displays itself as text.
class Button : public Label {
  public:
    Button(const char *text);
    virtual bool acceptsFocus() { return true; }
    bool pressed() { return pressed_; }
    virtual void update(Screen *screen);
    virtual bool handleInputEvent(int x, int y, bool selected, bool cancelled);
    #ifdef SCREENUI_DEBUG
    virtual char *description() { return "Button"; }
    #endif
  private:
    bool pressed_;
};

// A Component that displays either an on or off state. Clicking the component
// while it has focus causes it to change from on to off or vice-versa.
class Checkbox : public Label {
  public:
    Checkbox();
    bool checked() { return checked_; }
    virtual bool acceptsFocus() { return true; }
    virtual bool handleInputEvent(int x, int y, bool selected, bool cancelled);
    #ifdef SCREENUI_DEBUG
    virtual char *description() { return "Checkbox"; }
    #endif
  private:
    bool checked_;
};

// A Component that allows the user to scroll through several choices and
// select one. When the List is selected, future scroll events will cause it
// to scroll through it's selections. A select sets the current item
// or a cancel resets the list to it's previously selected item and
// releases control.
class List : public Label {
  public:
    List(uint8_t maxItems);
    void addItem(const char *item);
    const char *selectedItem() { return items_[selectedIndex_]; }
    uint8_t selectedIndex() { return selectedIndex_; }
    void setSelectedIndex(uint8_t selectedIndex);
    virtual bool acceptsFocus() { return true; }
    virtual bool handleInputEvent(int x, int y, bool selected, bool cancelled);
    #ifdef SCREENUI_DEBUG
    virtual char *description() { return "List"; }
    #endif
  private:
    char **items_;
    uint8_t itemCount_;
    uint8_t selectedIndex_;
};

// allows text input. Each character can be clicked to scroll through the alphabet.
class Input : public Label {
  public:
    Input(char *text);
    virtual void setText(char *text);
    virtual bool acceptsFocus() { return true; }
    virtual void paint(Screen *screen);
    virtual bool handleInputEvent(int x, int y, bool selected, bool cancelled);
    #ifdef SCREENUI_DEBUG
    virtual char *description() { return "Input"; }
    #endif
  protected:
    int8_t position_;
    bool selecting_;
};
		
// allows input of a floating point number.
class DecimalInput : public Input {
};

// allows input of an integer number with a certain base.
class IntegerInput : public Input {
};

// Component that allows the user to enter a time with up to three fields
// separated by semi-colons. e.g. 00, 00:00, 00:00:00
class TimeInput : public Input {
};

// A Container that allows the user to scroll through any number of rows of
// Components. The ScrollContainer can be thought of as a Screen with the same
// width as the Container it is added to, and an unlimited height.
// Components that will be added to the ScrollContainer should have their
// location set relative to their position in the ScrollContainer, not
// the main Screen.
class ScrollContainer : public Container {
  public:
    // We require a reference to Screen because we need focus information
    // during the dirty() check. This is a bit of a dirty hack, but it's
    // better than adding this property to every other object or walking
    // the tree to find it.
    ScrollContainer(Screen *screen, uint8_t width, uint8_t height);
    ~ScrollContainer();
    virtual void update(Screen *screen);
    virtual void add(Component *component, int8_t x, int8_t y);
    virtual void paint(Screen *screen);
    virtual bool dirty();
    #ifdef SCREENUI_DEBUG
    virtual char *description() { return "ScrollContainer"; }
    #endif
  private:
    bool scrollNeeded();
    void offsetChildren(int x, int y);
  
    Component *lastFocusHolder_;
    Screen *screen_;
    char *clearLine;
    bool firstUpdateCompleted_;
};

// A specialization of ScrollContainer that contains only Buttons and provides
// a simple API for managing the set of Buttons like a menu.
class Menu : public ScrollContainer {
};

#endif