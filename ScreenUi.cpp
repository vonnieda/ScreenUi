/**
 * A toolkit for building character based user interfaces on small displays.
 * 
 * Copyright (c) 2012 Jason von Nieda <jason@vonnieda.org>
 * This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 
 * Unported License. To view a copy of this license, visit 
 * http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to Creative 
 * Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.
 */

#include <ScreenUi.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef SCREENUI_DEBUG
#include <WProgram.h>
#endif

// TODO: Change to PROGMEM
uint8_t charCheckmark[] =     {0,     // B00000
                               0,     // B00000
                               1,     // B00001
                               2,     // B00010
                               20,    // B10100
                               8,     // B01000
                               0,     // B00000
                               0};    // B00000

                               
////////////////////////////////////////////////////////////////////////////////
// Screen
////////////////////////////////////////////////////////////////////////////////
Screen::Screen(uint8_t width, uint8_t height) {
  setSize(width, height);
  cleared_ = false;
  focusHolder_ = NULL;
  focusHolderSelected_ = false;
  createCustomChar(7, charCheckmark);
  annoyingBugWorkedAround_ = false;
}

void Screen::update() {
  if (!cleared_) {
    clear();
    cleared_ = true;
  }
  Container::update(this);
  int x, y;
  bool selected, cancelled;
  getInputDeltas(&x, &y, &selected, &cancelled);
  Component *oldFocusHolder = focusHolder_;
  if (x || y || selected || cancelled) {
    if (focusHolderSelected_) {
      focusHolderSelected_ = focusHolder_->handleInputEvent(x, y, selected, cancelled);
    }
    else {
      if (selected) {
        focusHolderSelected_ = focusHolder_->handleInputEvent(x, y, selected, cancelled);
      }
      else if (x || y) {
        // TODO: Make axis x or y configurable.
        if (y > 0) {
          focusHolder_ = nextFocusHolder(focusHolder_, false);
          if (!focusHolder_) {
            focusHolder_ = nextFocusHolder(focusHolder_, false);
          }
        }
        else if (y < 0) {
          focusHolder_ = nextFocusHolder(focusHolder_, true);
          if (!focusHolder_) {
            focusHolder_ = nextFocusHolder(focusHolder_, true);
          }
        }
      }
    }
  }
  if (focusHolder_ == NULL) {
    focusHolder_ = nextFocusHolder(focusHolder_, false);
  }
  if (oldFocusHolder != focusHolder_) {
    if (oldFocusHolder) {
      oldFocusHolder->repaint();
    }
    focusHolder_->repaint();
  }
  paint(this);
  moveCursor(cursorX_, cursorY_);
  
  // TODO: Bug I can't figure out. If we use the dirtyness system, the first paint
  // fails to draw anything to the screen and then subsequent ones don't get called
  // because they aren't dirty. The second paint works fine. 
  // Adding repaint() here allows us to paint during the second loop which gets
  // everything into a state where it works great, but I can't figure out why.
  // It doesn't seem to have to do with timing or the clear().
  if (!annoyingBugWorkedAround_) {
    repaint();
    annoyingBugWorkedAround_ = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Container
////////////////////////////////////////////////////////////////////////////////
Container::Container() {
  components_ = NULL;
  componentsLength_ = 0;
  componentCount_ = 0;
}

Container::~Container() {
  free(components_);
}

void Container::update(Screen *screen) {
  for (int i = 0; i < componentCount_; i++) {
    components_[i]->update(screen);
  }
}

void Container::paint(Screen *screen) {
  for (int i = 0; i < componentCount_; i++) {
    if (components_[i]->dirty()) {
      components_[i]->paint(screen);
    }
  }
}

void Container::repaint() {
  for (int i = 0; i < componentCount_; i++) {
    components_[i]->repaint();
  }
}

void Container::add(Component *component, int8_t x, int8_t y) {
  if (!components_ || componentsLength_ <= componentCount_) {
    uint8_t newComponentsLength = (componentsLength_ * 2) + 1;
    Component** newComponents = (Component**) malloc(newComponentsLength * sizeof(Component*));
    for (int i = 0; i < componentCount_; i++) {
      newComponents[i] = components_[i];
    }
    free(components_);
    components_ = newComponents;
    componentsLength_ = newComponentsLength;
  }
  components_[componentCount_++] = component;
  component->setLocation(x, y);
  component->repaint();
}

Component *Container::nextFocusHolder(Component *focusHolder, bool reverse) {
  bool focusHolderFound = false;
  return nextFocusHolder(focusHolder, reverse, &focusHolderFound);
}

Component *Container::nextFocusHolder(Component *focusHolder, bool reverse, bool *focusHolderFound) {
  for (int i = (reverse ? componentCount_ - 1 : 0); (reverse ? (i > 0) : (i < componentCount_)); (reverse ? i-- : i++)) {
    Component *c = components_[i];
    if (c->isContainer()) {
      Component *next = ((Container*) c)->nextFocusHolder(focusHolder, reverse, focusHolderFound);
      if (next) {
        return next;
      }
    }
    else {
      if (c->acceptsFocus()) {
        if (!focusHolder || *focusHolderFound) {
          return c;
        }
        else if (c == focusHolder) {
          *focusHolderFound = true;
        }
      }
    }
  }
  return NULL;
}

bool Container::dirty() {
  for (int i = 0; i < componentCount_; i++) {
    if (components_[i]->dirty()) {
      return true;
    }
  }
  return false;
}

bool Container::contains(Component *component) {
  for (int i = 0; i < componentCount_; i++) {
    Component *c = components_[i];
    if (c == component) {
      return true;
    }
    else if (c->isContainer()) {
      if (((Container*) c)->contains(component)) {
        return true;
      }
    }
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Component
////////////////////////////////////////////////////////////////////////////////

void Component::paint(Screen *screen) {
  dirty_ = false;
}

bool Component::dirty() {
  return dirty_;
}

////////////////////////////////////////////////////////////////////////////////
// Label
////////////////////////////////////////////////////////////////////////////////
Label::Label(const char *text) {
  setSize(0, 1);
  setText(text);
  captured_ = false;
  dirtyWidth_ = 0;
}

void Label::paint(Screen *screen) {
  Component::paint(screen);
  
  // Label does not accept focus, but Button, Checkbox and List are all
  // subclasses that want to share the same text drawing system, so we
  // just account for it here.
  if (acceptsFocus()) {
    if (screen->focusHolder() == this) {
      if (captured_) {
        screen->draw(x_, y_, ">");
        screen->draw(x_ + width_ + 1, y_, "<");
      }
      else {
        screen->draw(x_, y_, "<");
        screen->draw(x_ + width_ + 1, y_, ">");
      }
    }
    else {
      screen->draw(x_, y_, "[");
      screen->draw(x_ + width_ + 1, y_, "]");
    }
  }
  
  screen->draw(x_ + (acceptsFocus() ? 1 : 0), y_, text_);
  if (dirtyWidth_) {
    for (int i = 0; i < dirtyWidth_ - width_; i++) {
      screen->draw(x_ + width_ + i + (acceptsFocus() ? 2 : 0), y_, " ");
    }
    dirtyWidth_ = 0;
  }
}

void Label::setText(const char *text) {
  text_ = (char*) text;
  uint8_t newWidth = strlen(text);
  if (newWidth < width_) {
    dirtyWidth_ = width_;
  }
  width_ = newWidth;
  repaint();
}

////////////////////////////////////////////////////////////////////////////////
// Button
////////////////////////////////////////////////////////////////////////////////
Button::Button(const char *text) : Label(text) {
  setText(text);
  pressed_ = false;
}

void Button::update(Screen *screen) {
  pressed_ = false;
}

bool Button::handleInputEvent(int x, int y, bool selected, bool cancelled) {
  pressed_ = selected;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Checkbox
////////////////////////////////////////////////////////////////////////////////

Checkbox::Checkbox() : Label(" ") {
  checked_ = false;
}

bool Checkbox::handleInputEvent(int x, int y, bool selected, bool cancelled) {
  if (selected) {
    checked_ = !checked_;
    // Not James Bond. The 8th custom character location. By using a non-zero
    // location we can still send it via a string, which means we can still
    // be a Label instead of having a custom paint routine.
    setText(checked_ ? "\007" : " ");
    repaint();
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// List
////////////////////////////////////////////////////////////////////////////////

List::List(uint8_t maxItems) : Label(NULL) {
  items_ = (char **) malloc(maxItems * (sizeof(char*)));
  itemCount_ = 0;
  selectedIndex_ = 0;
  captured_ = false;
}

void List::addItem(const char *item) {
  items_[itemCount_++] = (char*) item;
  if (text_ == NULL) {
    setText(selectedItem());
  }
}

void List::setSelectedIndex(uint8_t selectedIndex) {
  selectedIndex_ = selectedIndex;
  setText(selectedItem());
  repaint();
}

bool List::handleInputEvent(int x, int y, bool selected, bool cancelled) {
  if (captured_ && y) {
    if (y < 0) {
      setSelectedIndex(max(selectedIndex_ + y, 0));
    }
    else {
      setSelectedIndex(min(selectedIndex_ + y, itemCount_ - 1));
    }
  }
  if (selected) {
    captured_ = !captured_;
    repaint();
  }
  return captured_;
}

////////////////////////////////////////////////////////////////////////////////
// Input
////////////////////////////////////////////////////////////////////////////////

// TODO: trim incoming text and after each return, right justify the text
Input::Input(char *text) : Label((const char*) text) {
  position_ = 0;
  selecting_ = false;
}

void Input::setText(char *text) {
  Label::setText((const char *) text);
  position_ = 0;
  selecting_ = false;
  repaint();
}

void Input::paint(Screen *screen) {
  Label::paint(screen);
  screen->setCursorVisible(captured_ && selecting_);
  screen->setBlink(captured_ && !selecting_);
  screen->setCursorLocation(x_ + position_ + 1, y_);
}

bool Input::handleInputEvent(int x, int y, bool selected, bool cancelled) {
  // If the input is captured and there has been a scroll event we're going to
  // either change the position or change the selection.
  if (captured_ && y) {
    // If we're changing the selection, scroll through the character set.
    if (selecting_) {
      // TODO: replace this with a selectable character set that makes more
      // sense
      if (y < 0) {
        text_[position_] = max(text_[position_] + y, ' ');
      }
      else {
        text_[position_] = min(text_[position_] + y, '}');
      }
    }
    // Otherwise we are changing the position. If the position is moving before
    // or after the field we release the input.
    else {
      position_ += y;
      if (position_ < 0 || position_ >= width_) {
        captured_ = false;
      }
    }
    repaint();
  }
  // If there has been a click we will either capture the input, 
  // start selection or end selection.
  if (selected) {
    // If input is captured we will start or end selection
    // input.
    if (captured_) {
      selecting_ = !selecting_;
    }
    // Capture the input
    else {
      captured_ = true;
      position_ = 0;
      selecting_ = false;
    }
    repaint();
  }
  return captured_;
}

////////////////////////////////////////////////////////////////////////////////
// ScrollContainer
////////////////////////////////////////////////////////////////////////////////

ScrollContainer::ScrollContainer(Screen *screen, uint8_t width, uint8_t height) {
  setSize(width, height);
  screen_ = screen;
  clearLine = (char*) malloc(width + 1);
  memset(clearLine, ' ', width);
  clearLine[width] = NULL;
  firstUpdateCompleted_ = false;
}

ScrollContainer::~ScrollContainer() {
  free(clearLine);
}

void ScrollContainer::add(Component *component, int8_t x, int8_t y) {
  Container::add(component, x, y);
  if (firstUpdateCompleted_) {
    // TODO: if the first update has already completed we need to update
    // incoming components locations as they are added
  }
}

bool ScrollContainer::dirty() {
  if (Container::dirty()) {
    return true;
  }
  return scrollNeeded();
}

void ScrollContainer::update(Screen *screen) {
  if (!firstUpdateCompleted_) {
    offsetChildren(0, y_);
    firstUpdateCompleted_ = true;
  }
}

void ScrollContainer::offsetChildren(int x, int y) {
  for (int i = 0; i < componentCount_; i++) {
    Component *c = components_[i];
    /*
    Serial.print("Moving ");
    Serial.print(c->description());
    Serial.print(" from ");
    Serial.print(c->x(), DEC);
    Serial.print(", ");
    Serial.print(c->y(), DEC);
    Serial.print(" to ");
    Serial.print(c->x() + x, DEC);
    Serial.print(", ");
    Serial.println(c->y() + y, DEC);
    */
    c->setLocation(c->x() + x, c->y() + y);
  }
}

bool ScrollContainer::scrollNeeded() {
  // see if the focus holder has changed since the last check
  Component *focusHolder = screen_->focusHolder();
  if (lastFocusHolder_ != focusHolder) {
    // it has, so see if the new focus holder is one of ours
    if (contains(focusHolder)) {
      // it is, so we need to be sure it's visible, which means it's
      // y position plus height has to be within our window of visibility
      // our window of visbility is our y_ + row_ to y_ + row_ + height_
      uint8_t yStart = y_;
      uint8_t yEnd = y_ + height_ - 1;
      if (focusHolder->y() < yStart || focusHolder->y() > yEnd) {
        // it is not currently visible, so we are dirty
        return true;
      }
    }
  }
  return false;
}

void ScrollContainer::paint(Screen *screen) {
  Component::paint(screen);
  if (scrollNeeded()) {
    // we need to scroll the window
    Component *focusHolder = screen_->focusHolder();
    // clear the window
    for (int i = 0; i < height_; i++) {
      screen->draw(x_, y_ + i, clearLine);
    }
    // set the new row_. if the new focus holder is below our currently
    // visible area we want to increment the row the minimum amount to make it
    // visible, and likewise if it is above, we want to decrement.
    // TODO: These are calculated in scrollNeeded(), see if it would be better
    // to reuse them somehow
    uint8_t yStart = y_;
    uint8_t yEnd = y_ + height_ - 1;
    if (focusHolder->y() > yEnd) {
      // if the component is below our visible window, increment the row count
      // by the difference between the bottom visible row and the y position
      // of the component
      offsetChildren(0, yEnd - focusHolder->y());
    }
    else {
      offsetChildren(0, yStart - focusHolder->y());
    }
    
    lastFocusHolder_ = screen->focusHolder();

    // tell all the children they need to be repainted. we will only paint
    // the ones that are now visible
    repaint();
  }
  for (int i = 0; i < componentCount_; i++) {
    Component *component = components_[i];
    if (component->dirty() && (component->y() >= y_) && (component->y() < y_ + height_)) {
      component->paint(screen);
    }
    else {
      component->clearDirty();
    }
  }
}

