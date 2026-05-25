// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief Testing canvas layout system
 */

#include "BoxLayout.hxx"
#include "GridLayout.hxx"
#include "NasalWidget.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/misc/test_macros.hxx>
#include <simgear/nasal/cppbind/NasalContext.hxx>

#include <cstdlib>

//------------------------------------------------------------------------------
namespace sc = simgear::canvas;

class TestWidget:
  public sc::LayoutItem
{
  public:
      TestWidget(const SGVec2i& min_size,
                 const SGVec2i& size_hint,
                 const SGVec2i& max_size = MAX_SIZE,
                 const SGVec2i& gridLoc = {0, 0},
                 const SGVec2i& span = {1, 1})
      {
          _size_hint = size_hint;
          _min_size = min_size;
          _max_size = max_size;
          _gridLocation = gridLoc;
          _span = span;
    }

    TestWidget(const TestWidget& rhs)
    {
      _size_hint = rhs._size_hint;
      _min_size = rhs._min_size;
      _max_size = rhs._max_size;
      _gridLocation = rhs._gridLocation;
      _span = rhs._span;
    }

    void setMinSize(const SGVec2i& size) { _min_size = size; }
    void setMaxSize(const SGVec2i& size) { _max_size = size; }
    void setSizeHint(const SGVec2i& size) { _size_hint = size; }

  protected:

    virtual SGVec2i sizeHintImpl() const { return _size_hint; }
    virtual SGVec2i minimumSizeImpl() const { return _min_size; }
    virtual SGVec2i maximumSizeImpl() const { return _max_size; }

    virtual void visibilityChanged(bool visible)
    {
      if( !visible )
        _geometry.set(0, 0, 0, 0);
    }
};

class TestWidgetHFW:
  public TestWidget
{
  public:
    TestWidgetHFW( const SGVec2i& min_size,
                   const SGVec2i& size_hint,
                   const SGVec2i& max_size = MAX_SIZE ):
      TestWidget(min_size, size_hint, max_size)
    {

    }

    virtual bool hasHeightForWidth() const
    {
      return true;
    }

    virtual int heightForWidthImpl(int w) const
    {
      return _size_hint.x() * _size_hint.y() / w;
    }

    virtual int minimumHeightForWidthImpl(int w) const
    {
      return _min_size.x() * _min_size.y() / w;
    }
};

typedef SGSharedPtr<TestWidget> TestWidgetRef;

//------------------------------------------------------------------------------
void test_horizontal_layout()
{
  sc::BoxLayoutRef box_layout(new sc::BoxLayout(sc::BoxLayout::BottomToTop));
  box_layout->setSpacing(5);

  SG_CHECK_EQUAL_NOSTREAM(box_layout->direction(), sc::BoxLayout::BottomToTop);
  SG_CHECK_EQUAL(box_layout->spacing(), 5);

  box_layout->setDirection(sc::BoxLayout::LeftToRight);
  box_layout->setSpacing(9);

  SG_CHECK_EQUAL_NOSTREAM(box_layout->direction(), sc::BoxLayout::LeftToRight);
  SG_CHECK_EQUAL(box_layout->spacing(), 9);

  TestWidgetRef fixed_size_widget( new TestWidget( SGVec2i(16, 16),
                                                   SGVec2i(16, 16),
                                                   SGVec2i(16, 16) ) );
  box_layout->addItem(fixed_size_widget);

  SG_CHECK_EQUAL(box_layout->minimumSize(), SGVec2i(16, 16));
  SG_CHECK_EQUAL(box_layout->sizeHint(), SGVec2i(16, 16));
  SG_CHECK_EQUAL(box_layout->maximumSize(), SGVec2i(16, 16));

  TestWidgetRef limited_resize_widget( new TestWidget( SGVec2i(16, 16),
                                                       SGVec2i(32, 32),
                                                       SGVec2i(256, 64) ) );
  box_layout->addItem(limited_resize_widget);

  // Combined sizes of both widget plus the padding between them
  SG_CHECK_EQUAL(box_layout->minimumSize(), SGVec2i(41, 16));
  SG_CHECK_EQUAL(box_layout->sizeHint(), SGVec2i(57, 32));
  SG_CHECK_EQUAL(box_layout->maximumSize(), SGVec2i(281, 64));

  // Test with different spacing/padding
  box_layout->setSpacing(5);

  SG_CHECK_EQUAL(box_layout->minimumSize(), SGVec2i(37, 16));
  SG_CHECK_EQUAL(box_layout->sizeHint(), SGVec2i(53, 32));
  SG_CHECK_EQUAL(box_layout->maximumSize(), SGVec2i(277, 64));

  box_layout->setGeometry(SGRecti(0, 0, 128, 32));

  // Fixed size for first widget and remaining space goes to second widget
  SG_CHECK_EQUAL(fixed_size_widget->geometry(), SGRecti(0, 8, 16, 16));
  SG_CHECK_EQUAL(limited_resize_widget->geometry(), SGRecti(21, 0, 107, 32));

  TestWidgetRef stretch_widget( new TestWidget( SGVec2i(16, 16),
                                                SGVec2i(32, 32),
                                                SGVec2i(128, 32) ) );
  box_layout->addItem(stretch_widget, 1);
  box_layout->update();

  SG_CHECK_EQUAL(box_layout->minimumSize(), SGVec2i(58, 16));
  SG_CHECK_EQUAL(box_layout->sizeHint(), SGVec2i(90, 32));
  SG_CHECK_EQUAL(box_layout->maximumSize(), SGVec2i(410, 64));

  // Due to the stretch factor only the last widget gets additional space. All
  // other widgets get the preferred size.
  SG_CHECK_EQUAL(fixed_size_widget->geometry(), SGRecti(0, 8, 16, 16));
  SG_CHECK_EQUAL(limited_resize_widget->geometry(), SGRecti(21, 0, 32, 32));
  SG_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(58, 0, 70, 32));

  // Test stretch factor
  TestWidgetRef fast_stretch( new TestWidget(*stretch_widget) );
  sc::BoxLayoutRef box_layout_stretch(
    new sc::BoxLayout(sc::BoxLayout::LeftToRight)
  );

  box_layout_stretch->addItem(stretch_widget, 1);
  box_layout_stretch->addItem(fast_stretch, 2);

  box_layout_stretch->setGeometry(SGRecti(0,0,128,32));

  SG_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(0, 0, 41, 32));
  SG_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(46, 0, 82, 32));

  box_layout_stretch->setGeometry(SGRecti(0,0,256,32));

  SG_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(0, 0, 123, 32));
  SG_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(128, 0, 128, 32));

  // Test superfluous space to padding
  box_layout_stretch->setGeometry(SGRecti(0,0,512,32));

  SG_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(83, 0, 128, 32));
  SG_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(300, 0, 128, 32));

  // ...and now with alignment
  //
  // All widgets without alignment get their maximum space and the remaining
  // space is equally distributed to the remaining items. All items with
  // alignment are set to their size hint and positioned according to their
  // alignment.

  // Left widget: size hint and positioned on the left
  // Right widget: maximum size and positioned on the right
  stretch_widget->setAlignment(sc::AlignLeft);
  box_layout_stretch->update();
  SG_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(0, 0, 32, 32));
  SG_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(384, 0, 128, 32));

  // Left widget: align right
  stretch_widget->setAlignment(sc::AlignRight);
  box_layout_stretch->update();
  SG_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(347, 0, 32, 32));
  SG_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(384, 0, 128, 32));

  // Left widget: size hint and positioned on the right
  // Right widget: size hint and positioned on the left of the right half
  fast_stretch->setAlignment(sc::AlignLeft);
  box_layout_stretch->update();
  SG_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(221, 0, 32, 32));
  SG_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(258, 0, 32, 32));

  // Also check vertical alignment
  stretch_widget->setAlignment(sc::AlignLeft | sc::AlignTop);
  fast_stretch->setAlignment(sc::AlignLeft | sc::AlignBottom);
  box_layout_stretch->setGeometry(SGRecti(0,0,512,64));
  SG_CHECK_EQUAL(stretch_widget->geometry(), SGRecti(0, 0, 32, 32));
  SG_CHECK_EQUAL(fast_stretch->geometry(), SGRecti(258, 32, 32, 32));
}

//------------------------------------------------------------------------------
// Test more space then preferred, but less than maximum
void test_hbox_pref_to_max()
{
  sc::BoxLayoutRef hbox(new sc::HBoxLayout());
  TestWidgetRef w1( new TestWidget( SGVec2i(16,   16),
                                    SGVec2i(32,   32),
                                    SGVec2i(9999, 32) ) ),
                w2( new TestWidget(*w1) );

  hbox->addItem(w1);
  hbox->addItem(w2);

  hbox->setGeometry( SGRecti(0, 0, 256, 32) );

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 126, 32));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(131, 0, 125, 32));

  hbox->setStretch(0, 1);
  hbox->setStretch(1, 1);

  SG_CHECK_EQUAL(hbox->stretch(0), 1);
  SG_CHECK_EQUAL(hbox->stretch(1), 1);

  hbox->update();

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 125, 32));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(130, 0, 126, 32));

  SG_VERIFY(hbox->setStretchFactor(w1, 2));
  SG_VERIFY(hbox->setStretchFactor(w2, 3));
  SG_CHECK_EQUAL(hbox->stretch(0), 2);
  SG_CHECK_EQUAL(hbox->stretch(1), 3);

  hbox->removeItem(w1);

  SG_VERIFY(!hbox->setStretchFactor(w1, 0));
}

//------------------------------------------------------------------------------
void test_spacer_layouting()
{
  sc::HBoxLayout hbox;
  TestWidgetRef w1( new TestWidget( SGVec2i(16, 16),
                                    SGVec2i(32, 32),
                                    SGVec2i(9999, 9999) ) ),
                w2( new TestWidget(*w1) );

  hbox.addItem(w1);
  hbox.addItem(w2);
  hbox.addStretch(1);

  SG_CHECK_EQUAL(hbox.minimumSize(), SGVec2i(37, 16));
  SG_CHECK_EQUAL(hbox.sizeHint(), SGVec2i(69, 32));
  SG_CHECK_EQUAL(hbox.maximumSize(), sc::LayoutItem::MAX_SIZE);

  hbox.setGeometry(SGRecti(0, 0, 256, 40));

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 32, 40));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(37, 0, 32, 40));

  // now center with increased spacing between both widgets
  hbox.insertStretch(0, 1);
  hbox.insertSpacing(2, 10);

  SG_CHECK_EQUAL(hbox.minimumSize(), SGVec2i(47, 16));
  SG_CHECK_EQUAL(hbox.sizeHint(), SGVec2i(79, 32));
  SG_CHECK_EQUAL(hbox.maximumSize(), sc::LayoutItem::MAX_SIZE);

  hbox.update();

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(88, 0, 32, 40));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(135, 0, 32, 40));
}

//------------------------------------------------------------------------------
void test_vertical_layout()
{
  sc::BoxLayout vbox(sc::BoxLayout::TopToBottom);
  vbox.setSpacing(7);

  TestWidgetRef fixed_size_widget( new TestWidget( SGVec2i(16, 16),
                                                   SGVec2i(16, 16),
                                                   SGVec2i(16, 16) ) );
  TestWidgetRef limited_resize_widget( new TestWidget( SGVec2i(16, 16),
                                                       SGVec2i(32, 32),
                                                       SGVec2i(256, 64) ) );

  vbox.addItem(fixed_size_widget);
  vbox.addItem(limited_resize_widget);

  SG_CHECK_EQUAL(vbox.minimumSize(), SGVec2i(16, 39));
  SG_CHECK_EQUAL(vbox.sizeHint(), SGVec2i(32, 55));
  SG_CHECK_EQUAL(vbox.maximumSize(), SGVec2i(256, 87));

  vbox.setGeometry(SGRecti(10, 20, 16, 55));

  SG_CHECK_EQUAL(fixed_size_widget->geometry(), SGRecti(10, 20, 16, 16));
  SG_CHECK_EQUAL(limited_resize_widget->geometry(), SGRecti(10, 43, 16, 32));

  vbox.setDirection(sc::BoxLayout::BottomToTop);
}

//------------------------------------------------------------------------------
void test_boxlayout_insert_remove()
{
  sc::BoxLayoutRef hbox( new sc::HBoxLayout );

  SG_CHECK_EQUAL(hbox->count(), 0);
  SG_VERIFY(!hbox->itemAt(0));
  SG_VERIFY(!hbox->takeAt(0));

  TestWidgetRef w1( new TestWidget( SGVec2i(16,   16),
                                    SGVec2i(32,   32),
                                    SGVec2i(9999, 32) ) ),
                w2( new TestWidget(*w1) );

  hbox->addItem(w1);
  SG_CHECK_EQUAL(hbox->count(), 1);
  SG_CHECK_EQUAL_NOSTREAM(hbox->itemAt(0), w1);
  SG_CHECK_EQUAL_NOSTREAM(w1->getParent(), hbox);

  hbox->insertItem(0, w2);
  SG_CHECK_EQUAL(hbox->count(), 2);
  SG_CHECK_EQUAL_NOSTREAM(hbox->itemAt(0), w2);
  SG_CHECK_EQUAL_NOSTREAM(hbox->itemAt(1), w1);
  SG_CHECK_EQUAL_NOSTREAM(w2->getParent(), hbox);

  hbox->removeItem(w2);
  SG_CHECK_EQUAL(hbox->count(), 1);
  SG_CHECK_EQUAL_NOSTREAM(hbox->itemAt(0), w1);
  SG_VERIFY(!w2->getParent());

  hbox->addItem(w2);
  SG_CHECK_EQUAL(hbox->count(), 2);
  SG_CHECK_EQUAL_NOSTREAM(w2->getParent(), hbox);

  hbox->clear();
  SG_CHECK_EQUAL(hbox->count(), 0);
  SG_VERIFY(!w1->getParent());
  SG_VERIFY(!w2->getParent());
}

//------------------------------------------------------------------------------
void test_boxlayout_visibility()
{
  sc::BoxLayoutRef hbox( new sc::HBoxLayout );
  TestWidgetRef w1( new TestWidget( SGVec2i(16, 16),
                                    SGVec2i(32, 32) ) ),
                w2( new TestWidget(*w1) ),
                w3( new TestWidget(*w1) );

  hbox->addItem(w1);
  hbox->addItem(w2);
  hbox->addItem(w3);

  SG_CHECK_EQUAL(hbox->sizeHint().x(), 3 * 32 + 2 * hbox->spacing());

  hbox->setGeometry(SGRecti(0, 0, 69, 32));

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 20, 32));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(25, 0, 20, 32));
  SG_CHECK_EQUAL(w3->geometry(), SGRecti(50, 0, 19, 32));

  w2->setVisible(false);

  SG_VERIFY(hbox->isVisible());
  SG_VERIFY(w1->isVisible());
  SG_VERIFY(!w2->isVisible());
  SG_VERIFY(w2->isExplicitlyHidden());
  SG_VERIFY(w3->isVisible());

  SG_CHECK_EQUAL(hbox->sizeHint().x(), 2 * 32 + 1 * hbox->spacing());

  hbox->update();

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 32, 32));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(0, 0, 0, 0));
  SG_CHECK_EQUAL(w3->geometry(), SGRecti(37, 0, 32, 32));

  hbox->setVisible(false);

  SG_VERIFY(!hbox->isVisible());
  SG_VERIFY(hbox->isExplicitlyHidden());
  SG_VERIFY(!w1->isVisible());
  SG_VERIFY(!w1->isExplicitlyHidden());
  SG_VERIFY(!w2->isVisible());
  SG_VERIFY(w2->isExplicitlyHidden());
  SG_VERIFY(!w3->isVisible());
  SG_VERIFY(!w3->isExplicitlyHidden());

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 0, 0));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(0, 0, 0, 0));
  SG_CHECK_EQUAL(w3->geometry(), SGRecti(0, 0, 0, 0));

  w2->setVisible(true);

  SG_VERIFY(!w2->isVisible());
  SG_VERIFY(!w2->isExplicitlyHidden());

  hbox->setVisible(true);

  SG_VERIFY(hbox->isVisible());
  SG_VERIFY(w1->isVisible());
  SG_VERIFY(w2->isVisible());
  SG_VERIFY(w3->isVisible());

  hbox->update();

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 20, 32));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(25, 0, 20, 32));
  SG_CHECK_EQUAL(w3->geometry(), SGRecti(50, 0, 19, 32));
}


//------------------------------------------------------------------------------
void test_boxlayout_equal()
{
    sc::BoxLayoutRef hbox( new sc::HBoxLayout );
    TestWidgetRef w1(new TestWidget(SGVec2i(16, 16),
                                    SGVec2i(32, 32),
                                    SGVec2i(9999, 9999))),
        w2(new TestWidget(*w1)),
        w3(new TestWidget(*w1));


    w2->setMinSize(SGVec2i(40, 12));
    w2->setSizeHint(SGVec2i(60, 40));

    hbox->addItem(w1);
    hbox->addItem(w2);
    hbox->addItem(w3);

    hbox->setEqualsItem(w1);
    hbox->setEqualsItem(w2);
    hbox->setStretchFactor(w3, 1);

    SG_CHECK_EQUAL(hbox->minimumSize(), SGVec2i(106, 16));
    SG_CHECK_EQUAL(hbox->sizeHint(), SGVec2i(162, 40));

    hbox->setGeometry(SGRecti(0, 0, 256, 40));

    SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 60, 40));
    SG_CHECK_EQUAL(w2->geometry(), SGRecti(65, 0, 60, 40));
    SG_CHECK_EQUAL(w3->geometry(), SGRecti(130, 0, 126, 40));

    // visibility
    w2->setVisible(false);

    SG_CHECK_EQUAL(hbox->minimumSize(), SGVec2i(37, 16));
    SG_CHECK_EQUAL(hbox->sizeHint(), SGVec2i(69, 32));
}

//------------------------------------------------------------------------------
void test_boxlayout_contents_margins()
{
  sc::Margins m;

  SG_VERIFY(m.isNull());

  m = sc::Margins(5);

  SG_CHECK_EQUAL(m.l, 5);
  SG_CHECK_EQUAL(m.t, 5);
  SG_CHECK_EQUAL(m.r, 5);
  SG_CHECK_EQUAL(m.b, 5);

  m = sc::Margins(6, 7);

  SG_CHECK_EQUAL(m.l, 6);
  SG_CHECK_EQUAL(m.t, 7);
  SG_CHECK_EQUAL(m.r, 6);
  SG_CHECK_EQUAL(m.b, 7);

  SG_CHECK_EQUAL(m.horiz(), 12);
  SG_CHECK_EQUAL(m.vert(), 14);
  SG_VERIFY(!m.isNull());

  m = sc::Margins(1, 2, 3, 4);

  SG_CHECK_EQUAL(m.l, 1);
  SG_CHECK_EQUAL(m.t, 2);
  SG_CHECK_EQUAL(m.r, 3);
  SG_CHECK_EQUAL(m.b, 4);

  SG_CHECK_EQUAL(m.horiz(), 4);
  SG_CHECK_EQUAL(m.vert(), 6);
  SG_CHECK_EQUAL(m.size(), SGVec2i(4, 6));

  sc::BoxLayoutRef hbox( new sc::HBoxLayout );

  hbox->setContentsMargins(5, 10, 15, 20);

  SG_CHECK_EQUAL(hbox->minimumSize(), SGVec2i(20, 30));
  SG_CHECK_EQUAL(hbox->sizeHint(), SGVec2i(20, 30));
  SG_CHECK_EQUAL(hbox->maximumSize(), SGVec2i(20, 30));

  hbox->setGeometry(SGRecti(0, 0, 30, 40));

  SG_CHECK_EQUAL(hbox->geometry(), SGRecti(0, 0, 30, 40));
  SG_CHECK_EQUAL(hbox->contentsRect(), SGRecti(5, 10, 10, 10));

  TestWidgetRef w1( new TestWidget( SGVec2i(16, 16),
                                    SGVec2i(32, 32) ) ),
                w2( new TestWidget(*w1) ),
                w3( new TestWidget(*w1) );

  w1->setContentsMargin(5);
  w2->setContentsMargin(6);
  w3->setContentsMargin(7);

  SG_CHECK_EQUAL(w1->minimumSize(), SGVec2i(26, 26));
  SG_CHECK_EQUAL(w1->sizeHint(), SGVec2i(42, 42));
  SG_CHECK_EQUAL(w1->maximumSize(), sc::LayoutItem::MAX_SIZE);

  SG_CHECK_EQUAL(w2->minimumSize(), SGVec2i(28, 28));
  SG_CHECK_EQUAL(w2->sizeHint(), SGVec2i(44, 44));
  SG_CHECK_EQUAL(w2->maximumSize(), sc::LayoutItem::MAX_SIZE);

  SG_CHECK_EQUAL(w3->minimumSize(), SGVec2i(30, 30));
  SG_CHECK_EQUAL(w3->sizeHint(), SGVec2i(46, 46));
  SG_CHECK_EQUAL(w3->maximumSize(), sc::LayoutItem::MAX_SIZE);

  hbox->addItem(w1);
  hbox->addItem(w2);
  hbox->addItem(w3);

  SG_CHECK_EQUAL(hbox->minimumSize(), SGVec2i(114, 60));
  SG_CHECK_EQUAL(hbox->sizeHint(), SGVec2i(162, 76));
  SG_CHECK_EQUAL(hbox->maximumSize(), sc::LayoutItem::MAX_SIZE);

  hbox->setGeometry(SGRecti(0, 0, hbox->sizeHint().x(), hbox->sizeHint().y()));

  SG_CHECK_EQUAL(hbox->contentsRect(), SGRecti(5, 10, 142, 46));

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(5, 10, 42, 46));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(52, 10, 44, 46));
  SG_CHECK_EQUAL(w3->geometry(), SGRecti(101, 10, 46, 46));

  SG_CHECK_EQUAL(w1->contentsRect(), SGRecti(10, 15, 32, 36));
  SG_CHECK_EQUAL(w2->contentsRect(), SGRecti(58, 16, 32, 34));
  SG_CHECK_EQUAL(w3->contentsRect(), SGRecti(108, 17, 32, 32));
}

//------------------------------------------------------------------------------
void test_boxlayout_hfw()
{
  TestWidgetRef w1( new TestWidgetHFW( SGVec2i(16,   16),
                                       SGVec2i(32,   32) ) ),
                w2( new TestWidgetHFW( SGVec2i(24,   24),
                                       SGVec2i(48,   48) ) );

  SG_CHECK_EQUAL(w1->heightForWidth(16), 64);
  SG_CHECK_EQUAL(w1->minimumHeightForWidth(16), 16);
  SG_CHECK_EQUAL(w2->heightForWidth(24), 96);
  SG_CHECK_EQUAL(w2->minimumHeightForWidth(24), 24);

  TestWidgetRef w_no_hfw( new TestWidget( SGVec2i(16,   16),
                                          SGVec2i(32,   32) ) );
  SG_VERIFY(!w_no_hfw->hasHeightForWidth());
  SG_CHECK_EQUAL(w_no_hfw->heightForWidth(16), -1);
  SG_CHECK_EQUAL(w_no_hfw->minimumHeightForWidth(16), -1);

  // horizontal
  sc::HBoxLayout hbox;
  hbox.setSpacing(5);
  hbox.addItem(w1);
  hbox.addItem(w2);

  SG_CHECK_EQUAL(hbox.heightForWidth(45), w2->heightForWidth(24));
  SG_CHECK_EQUAL(hbox.heightForWidth(85), w2->heightForWidth(48));

  hbox.addItem(w_no_hfw);

  SG_CHECK_EQUAL(hbox.heightForWidth(66), 96);
  SG_CHECK_EQUAL(hbox.heightForWidth(122), 48);
  SG_CHECK_EQUAL(hbox.minimumHeightForWidth(66), 24);
  SG_CHECK_EQUAL(hbox.minimumHeightForWidth(122), 16);

  hbox.setGeometry(SGRecti(0, 0, 66, 24));

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 16, 24));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(21, 0, 24, 24));
  SG_CHECK_EQUAL(w_no_hfw->geometry(), SGRecti(50, 0, 16, 24));

  // vertical
  sc::VBoxLayout vbox;
  vbox.setSpacing(5);
  vbox.addItem(w1);
  vbox.addItem(w2);

  SG_CHECK_EQUAL(vbox.heightForWidth(24), 143);
  SG_CHECK_EQUAL(vbox.heightForWidth(48), 74);
  SG_CHECK_EQUAL(vbox.minimumHeightForWidth(24), 39);
  SG_CHECK_EQUAL(vbox.minimumHeightForWidth(48), 22);

  vbox.addItem(w_no_hfw);

  SG_CHECK_EQUAL(vbox.heightForWidth(24), 180);
  SG_CHECK_EQUAL(vbox.heightForWidth(48), 111);
  SG_CHECK_EQUAL(vbox.minimumHeightForWidth(24), 60);
  SG_CHECK_EQUAL(vbox.minimumHeightForWidth(48), 43);

  SGVec2i min_size = vbox.minimumSize(),
          size_hint = vbox.sizeHint();

  SG_CHECK_EQUAL(min_size, SGVec2i(24, 66));
  SG_CHECK_EQUAL(size_hint, SGVec2i(48, 122));

  vbox.setGeometry(SGRecti(0, 0, 24, 122));

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 24, 33));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(0, 38, 24, 47));
  SG_CHECK_EQUAL(w_no_hfw->geometry(), SGRecti(0, 90, 24, 32));

  // Vertical layout modifies size hints, so check if they are correctly
  // restored
  SG_CHECK_EQUAL(min_size, vbox.minimumSize());
  SG_CHECK_EQUAL(size_hint, vbox.sizeHint());

  vbox.setGeometry(SGRecti(0, 0, 50, 122));

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 50, 25));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(0, 30, 50, 51));
  SG_CHECK_EQUAL(w_no_hfw->geometry(), SGRecti(0, 86, 50, 36));

  // Same geometry as before -> should get same widget geometry
  // (check internal size hint cache updates correctly)
  vbox.setGeometry(SGRecti(0, 0, 24, 122));

  SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 24, 33));
  SG_CHECK_EQUAL(w2->geometry(), SGRecti(0, 38, 24, 47));
  SG_CHECK_EQUAL(w_no_hfw->geometry(), SGRecti(0, 90, 24, 32));
}

//------------------------------------------------------------------------------
void test_item_alignment_rect()
{
  TestWidgetRef w1( new TestWidget( SGVec2i(16, 16),
                                    SGVec2i(32, 32) ) );

  const SGRecti r(10, 10, 64, 64);

  // Default: AlignFill -> fill up to maximum size
  SG_CHECK_EQUAL(w1->alignmentRect(r), r);

  // Horizontal

  // AlignLeft -> width from size hint, positioned on the left
  w1->setAlignment(sc::AlignLeft);
  SG_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(10, 10, 32, 64));

  // AlignRight -> width from size hint, positioned on the left
  w1->setAlignment(sc::AlignRight);
  SG_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(42, 10, 32, 64));

  // AlignHCenter -> width from size hint, positioned in the center
  w1->setAlignment(sc::AlignHCenter);
  SG_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(26, 10, 32, 64));


  // Vertical

  // AlignTop -> height from size hint, positioned on the top
  w1->setAlignment(sc::AlignTop);
  SG_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(10, 10, 64, 32));

  // AlignBottom -> height from size hint, positioned on the bottom
  w1->setAlignment(sc::AlignBottom);
  SG_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(10, 42, 64, 32));

  // AlignVCenter -> height from size hint, positioned in the center
  w1->setAlignment(sc::AlignVCenter);
  SG_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(10, 26, 64, 32));


  // Vertical + Horizontal
  w1->setAlignment(sc::AlignCenter);
  SG_CHECK_EQUAL(w1->alignmentRect(r), SGRecti(26, 26, 32, 32));
}

//------------------------------------------------------------------------------
// TODO extend to_nasal_helper for automatic argument conversion
static naRef f_Widget_visibilityChanged(nasal::CallContext ctx)
{
  sc::NasalWidget* w = ctx.from_nasal<sc::NasalWidget*>(ctx.me);

  if( !ctx.requireArg<bool>(0) )
    w->setGeometry(SGRecti(0, 0, -1, -1));

  return naNil();
}

//------------------------------------------------------------------------------
void test_nasal_widget()
{
  nasal::Context c;
  nasal::Hash globals = c.newHash();

  nasal::Object::setupGhost();
  nasal::Ghost<sc::LayoutItemRef>::init("LayoutItem");
  sc::NasalWidget::setupGhost(globals);

  nasal::Hash me = c.newHash();
  me.set("visibilityChanged", &f_Widget_visibilityChanged);
  sc::NasalWidgetRef w( new sc::NasalWidget(me.get_naRef()) );

  // Default layout sizes (no user set values)
  SG_CHECK_EQUAL(w->minimumSize(), SGVec2i(16, 16));
  SG_CHECK_EQUAL(w->sizeHint(), SGVec2i(32, 32));
  SG_CHECK_EQUAL(w->maximumSize(), sc::LayoutItem::MAX_SIZE);

  // Changed layout sizes
  w->setLayoutMinimumSize( SGVec2i(2, 12) );
  w->setLayoutSizeHint(    SGVec2i(3, 13) );
  w->setLayoutMaximumSize( SGVec2i(4, 14) );

  SG_CHECK_EQUAL(w->minimumSize(), SGVec2i(2, 12));
  SG_CHECK_EQUAL(w->sizeHint(), SGVec2i(3, 13));
  SG_CHECK_EQUAL(w->maximumSize(), SGVec2i(4, 14));

  // User set values (overwrite layout sizes)
  w->setMinimumSize( SGVec2i(15, 16) );
  w->setSizeHint(    SGVec2i(17, 18) );
  w->setMaximumSize( SGVec2i(19, 20) );

  SG_CHECK_EQUAL(w->minimumSize(), SGVec2i(15, 16));
  SG_CHECK_EQUAL(w->sizeHint(), SGVec2i(17, 18));
  SG_CHECK_EQUAL(w->maximumSize(), SGVec2i(19, 20));

  // Only vertical user set values (layout/default for horizontal hints)
  w->setMinimumSize( SGVec2i(0, 21) );
  w->setSizeHint(    SGVec2i(0, 22) );
  w->setMaximumSize( SGVec2i(SGLimits<int>::max(), 23) );

  SG_CHECK_EQUAL(w->minimumSize(), SGVec2i(2, 21));
  SG_CHECK_EQUAL(w->sizeHint(), SGVec2i(3, 22));
  SG_CHECK_EQUAL(w->maximumSize(), SGVec2i(4, 23));

  w->setVisible(false);
  SG_CHECK_EQUAL(w->geometry(), SGRecti(0, 0, -1, -1));
}

//------------------------------------------------------------------------------
void test_gridlayout_layout()
{
    sc::GridLayoutRef grid(new sc::GridLayout);

    SG_CHECK_EQUAL(grid->count(), 0);
    SG_VERIFY(!grid->itemAt(0));
    SG_VERIFY(!grid->takeAt(0));

    TestWidgetRef w1(new TestWidget(SGVec2i(16, 16),
                                    SGVec2i(32, 32),
                                    SGVec2i(9999, 100))),
        w2(new TestWidget(*w1)),
        w3(new TestWidget(*w1)),
        w4(new TestWidget(*w1));

    w1->setMaxSize({9999, 32});
    grid->addItem(w1);
    SG_CHECK_EQUAL(grid->count(), 1);
    SG_CHECK_EQUAL_NOSTREAM(grid->itemAt(0), w1);
    SG_CHECK_EQUAL_NOSTREAM(w1->getParent(), grid);

    grid->addItem(w2, 1, 1);
    grid->addItem(w3, 2, 1);
    grid->addItem(w4, 0, 2, 3 /* col span */, 1);

    grid->setGeometry(SGRecti(0, 0, 160, 130));

    SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 4, 50, 32));
    SG_CHECK_EQUAL(w2->geometry(), SGRecti(55, 45, 50, 40));

    // check width includes padding of spanned columns
    SG_CHECK_EQUAL(w4->geometry(), SGRecti(0, 90, 160, 40));
}

//------------------------------------------------------------------------------
void test_gridlayout_min_size_layout()
{
    sc::GridLayoutRef grid(new sc::GridLayout);
    grid->setSpacing(4);

    TestWidgetRef w1(new TestWidget(SGVec2i(16, 16),
                                    SGVec2i(32, 32),
                                    SGVec2i(9999, 96))),
        w2(new TestWidget(*w1)),
        w3(new TestWidget(*w1)),
        w4(new TestWidget(*w1));

    w1->setMinSize({72, 24});
    w1->setSizeHint({72, 32});
    w1->setGridSpan({1, 2});

    w2->setMinSize({72, 48});
    w2->setSizeHint({96, 64});

    grid->addItem(w1);
    grid->addItem(w2, 1, 1);
    grid->addItem(w3, 2, 1);
    grid->addItem(w4, 0, 2, 2 /* col span */, 1);

    grid->setGeometry(SGRecti(0, 0, 248, 148));

    SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 85, 96));
    SG_CHECK_EQUAL(w2->geometry(), SGRecti(89, 18, 109, 78));
    SG_CHECK_EQUAL(w4->geometry(), SGRecti(0, 100, 198, 46));
}

//------------------------------------------------------------------------------
void test_gridlayout_stretch()
{
    sc::GridLayoutRef grid(new sc::GridLayout);
    grid->setSpacing(4);
    grid->setDimensions({3, 3});

    TestWidgetRef w1(new TestWidget(SGVec2i(16, 16),
                                    SGVec2i(32, 32),
                                    SGVec2i(9999, 9999))),
        w2(new TestWidget(*w1)),
        w3(new TestWidget(*w1)),
        w4(new TestWidget(*w1)),
        w5(new TestWidget(*w1));


    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 2);

    grid->setRowStretch(0, 1);
    grid->setRowStretch(1, 4);
    grid->setRowStretch(2, 1);

    w1->setGridSpan({1, 2});

    grid->addItem(w1);
    grid->addItem(w2, 1, 1);
    grid->addItem(w3, 2, 1);
    grid->addItem(w4, 0, 2, 2 /* col span */, 1);
    grid->addItem(w5, 2, 0);

    grid->setGeometry(SGRecti(0, 0, 248, 224));

    SG_CHECK_EQUAL(w1->geometry(), SGRecti(0, 0, 32, 168));
    SG_CHECK_EQUAL(w2->geometry(), SGRecti(36, 56, 80, 112));
    SG_CHECK_EQUAL(w4->geometry(), SGRecti(0, 172, 116, 52));
    SG_CHECK_EQUAL(w5->geometry(), SGRecti(120, 0, 128, 52));
}

int main()
{
    test_horizontal_layout();
    test_hbox_pref_to_max();
    test_spacer_layouting();
    test_vertical_layout();
    test_boxlayout_insert_remove();
    test_boxlayout_visibility();
    test_boxlayout_equal();
    test_boxlayout_contents_margins();
    test_boxlayout_hfw();
    test_item_alignment_rect();
    test_nasal_widget();
    test_gridlayout_layout();
    test_gridlayout_min_size_layout();
    test_gridlayout_stretch();
    return 0;
}
