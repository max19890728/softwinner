/*******************************************************************************
 * Copyright (c), 2020, Ultracker Tech. All rights reserved.
 ******************************************************************************/

#pragma once

#include <memory>

#include "UIKit/Class/view_controller.h"
#include "UIKit/Component/table_view.h"

namespace UI {

class TableViewControllerDelegate {};

class TableViewController : public UI::ViewController,
                            public UI::TableViewDelegate,
                            public UI::TableViewDataSource {
 public:
  static std::shared_ptr<TableViewController> init(UI::Coder = {});

 public:
  std::shared_ptr<TableViewControllerDelegate> delegate = nullptr;

 protected:
  std::shared_ptr<UI::TableView> table_view_;

 public:
  TableViewController();

  void Layout(UI::Coder) override;

  void ViewDidLoad() override;

  void ViewDidAppear() override;

  int NumberOfSectionIn() override;

  int NumberOfRowsInSection(int) override;

  void DidSelectRowAt(UI::IndexPath) override;

  std::shared_ptr<UI::TableViewCell> CellForRowAt(UI::IndexPath) override;
};
}  // namespace UI

typedef std::shared_ptr<UI::TableViewController> UITableViewController;
