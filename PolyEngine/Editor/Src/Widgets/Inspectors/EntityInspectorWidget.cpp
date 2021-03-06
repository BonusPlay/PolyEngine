#include "PolyEditorPCH.hpp"

#include <sstream>

#include "Managers/CommandsImpl.hpp"

//		general
//------------------------------------------------------------------------------
EntityInspectorWidget::EntityInspectorWidget(QWidget* parent, InspectorManager* mgr)
	: InspectorWidgetBase(parent, mgr)
{
	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &EntityInspectorWidget::customContextMenuRequested, this, &EntityInspectorWidget::SpawnContextMenu);

	// main layput
	MainLayout = new QGridLayout(this);
	MainLayout->setColumnStretch(0, 5);
	MainLayout->setColumnStretch(1, 15);
	MainLayout->setColumnStretch(2, 2);

	// context menu
	ContextMenu = new QMenu(this);
		
		AddComponentAction = new QAction("Add Components", this);
		ContextMenu->addAction(AddComponentAction);
		connect(AddComponentAction, &QAction::triggered, this, &EntityInspectorWidget::AddComponent);

		RemoveComponentAction = new QAction("Remove Components", this);
		ContextMenu->addAction(RemoveComponentAction);
		connect(RemoveComponentAction, &QAction::triggered, this, &EntityInspectorWidget::RemoveComponent);
	
	// TODO(squares): find better way
	for (size_t i = 0; i < MAX_COMPONENTS_COUNT + 6; ++i)
		MainLayout->setRowStretch((int)i, 1);
	
	
	// Name
	NameField = new StringControl(this);
	NameField->SetName("Name");
	NameField->SetToolTip("String");
	MainLayout->addWidget(NameField, 0, 0, 1, 2);
	connect(NameField, &StringControl::ObjectUpdated, this, &EntityInspectorWidget::ControlObjectUpdated);
	
	// UniqueID
	UniqueIdText = new QLabel(this);
	UniqueIdText->setText("UniqueID");
	MainLayout->addWidget(UniqueIdText, 1, 0);
	
	UniqueIdField = new QLineEdit(this);
	UniqueIdField->setDisabled(true);
	MainLayout->addWidget(UniqueIdField, 1, 1);
	
	// ParentID / Name
	ParentIdNameText = new QLabel(this);
	ParentIdNameText->setText("Name <Parent ID>");
	MainLayout->addWidget(ParentIdNameText, 2, 0);
	
	ParentIdNameField = new QLineEdit(this);
	ParentIdNameField->setDisabled(true);
	MainLayout->addWidget(ParentIdNameField, 2, 1);
	
	ParentSelectButton = new QPushButton(this);
	ParentSelectButton->setText("Select");
	MainLayout->addWidget(ParentSelectButton, 2, 2);
	connect(ParentSelectButton, &QPushButton::clicked, this, &EntityInspectorWidget::SelectParent);

	// ChildrenID / Name
	ChildrenIdNameText = new QLabel(this);
	ChildrenIdNameText->setText("Name / Child ID");
	MainLayout->addWidget(ChildrenIdNameText, 3, 0);
	
	ChildrenIdNameField = new QComboBox(this);
	MainLayout->addWidget(ChildrenIdNameField, 3, 1);
	
	ChildrenSelectButton = new QPushButton(this);
	ChildrenSelectButton->setText("Select");
	MainLayout->addWidget(ChildrenSelectButton, 3, 2);
	connect(ChildrenSelectButton, &QPushButton::clicked, this, &EntityInspectorWidget::SelectChild);

	// Transform
	TransformSection = new SectionContainer("Transform", this);
	Transform = new TransformControl(this);
	Transform->SetName("Transform");
	Transform->SetToolTip("EntityTransform");
	connect(Transform, &TransformControl::ObjectUpdated, this, &EntityInspectorWidget::ControlObjectUpdated);
	TransformSection->SetWidget(Transform);

	MainLayout->addWidget(TransformSection, 4, 0, 1, 3);

	EntitiesSelectionChanged();
}

//------------------------------------------------------------------------------
void EntityInspectorWidget::InitializeConnections()
{
	connect(Manager, &InspectorManager::EntitiesSpawned, this, &EntityInspectorWidget::Update);
	connect(Manager, &InspectorManager::EntitiesDestroyed, this, &EntityInspectorWidget::Update);
	connect(Manager, &InspectorManager::EntitiesModified, this, &EntityInspectorWidget::Update);
	connect(Manager, &InspectorManager::EntitiesReparented, this, &EntityInspectorWidget::Update);
	connect(Manager, &InspectorManager::EntitiesSelectionChanged, this, &EntityInspectorWidget::EntitiesSelectionChanged);

	connect(Manager, &InspectorManager::ComponentsAdded, this, &EntityInspectorWidget::Reload);
	connect(Manager, &InspectorManager::ComponentsRemoved, this, &EntityInspectorWidget::Reload);

	connect(Manager, &InspectorManager::Update, this, &EntityInspectorWidget::Update);
}

//------------------------------------------------------------------------------
void EntityInspectorWidget::Reset()
{
	for (auto cmp : ComponentSections)
	{
		MainLayout->removeWidget(cmp);
		delete cmp;
	}
	ComponentSections.Clear();
	ComponentInspectors.Clear();

	NameField->Reset();
	NameField->SetDisableEdit(true);

	UniqueIdField->setText("");

	ParentIdNameField->setText("");
	ParentSelectButton->hide();

	ChildrenIdNameField->clear();
	ChildrenSelectButton->hide();

	Transform->Reset();
	TransformSection->hide();
}



//		slots
//------------------------------------------------------------------------------
void EntityInspectorWidget::EntitiesSelectionChanged()
{
	Dynarray<Entity*> selectedEntities = Manager->GetSelectedEntities();

	// remove all old entity component sections
	for (auto cmp : ComponentSections)
	{
		MainLayout->removeWidget(cmp);
		delete cmp;
	}
	ComponentSections.Clear();
	ComponentInspectors.Clear();

	// no entity selected
	if (selectedEntities.GetSize() == 0)
	{
		NameField->Reset();
		NameField->SetText("");
		NameField->SetDisableEdit(true);

		UniqueIdField->setText("");

		ParentIdNameField->setText("");
		ParentSelectButton->hide();

		ChildrenIdNameField->clear();
		ChildrenSelectButton->hide();

		Transform->Reset();
		TransformSection->hide();
	}
	else if (selectedEntities.GetSize() > 1)
	{
		NameField->Reset();
		NameField->SetText("< multiple selection >");
		NameField->SetDisableEdit(true);

		UniqueIdField->setText("< multiple selection >");


		bool sameParent = true;
		Entity* parent = selectedEntities[0]->GetParent();

		for (Entity* e : selectedEntities)
			if (e->GetParent() != parent)
			{
				sameParent = false;
				break;
			}


		if (sameParent)
		{
			std::stringstream ss;
			ss.str(std::string());
			ss << selectedEntities[0]->GetParent()->GetName() + "\t<";
			ss << selectedEntities[0]->GetParent()->GetUUID();
			ss << "> ";
			ParentIdNameField->setText(&ss.str()[0]);
			ParentSelectButton->show();
		}
		else
		{
			ParentIdNameField->setText("< multiple selection >");
			ParentSelectButton->hide();
		}

		ChildrenIdNameField->clear();
		ChildrenSelectButton->hide();

		Transform->Reset();
		TransformSection->hide();
	}
	else
	{
		std::stringstream ss;

		// name
		void* ptr = ((char*)selectedEntities[0]) + selectedEntities[0]->GetPropertyManager()->GetPropertyList()[2].Offset;
		NameField->SetObject(ptr);
		NameField->SetDisableEdit(false);

		// id
		ss << selectedEntities[0]->GetUUID();
		UniqueIdField->setText(&ss.str()[0]);

		// parent
		if (selectedEntities[0]->IsRoot())
		{
			ParentIdNameField->setText("");
			ParentSelectButton->hide();
		}
		else
		{
			ss.str(std::string());
			ss << selectedEntities[0]->GetParent()->GetName() + "\t<";
			ss << selectedEntities[0]->GetParent()->GetUUID();
			ss << "> ";
			ParentIdNameField->setText(&ss.str()[0]);
			ParentSelectButton->show();
		}

		//children
		ChildrenIdNameField->clear();
		for (auto& child : selectedEntities[0]->GetChildren())
		{
			ss.str(std::string());
			ss << child->GetName() + " <";
			ss << child->GetUUID();
			ss << "> ";
			ChildrenIdNameField->addItem(&ss.str()[0]);
		}
		if ((int)selectedEntities[0]->GetChildren().GetSize() > 0)
			ChildrenSelectButton->show();
		else
			ChildrenSelectButton->hide();

		// transform
		if (selectedEntities[0]->IsRoot())
		{
			TransformSection->hide();
			Transform->Reset();
		}
		else
		{
			TransformSection->show();
			void* ptr = ((char*)selectedEntities[0]) + selectedEntities[0]->GetPropertyManager()->GetPropertyList()[3].Offset;
			Transform->SetObject(ptr);
		}

		// components
		ReloadComponentSections();
	}
}

//------------------------------------------------------------------------------
void EntityInspectorWidget::ComponentsAdded()
{
	ReloadComponentSections();
}

//------------------------------------------------------------------------------
void EntityInspectorWidget::ComponentsRemoved()
{
	ReloadComponentSections();
}

//------------------------------------------------------------------------------
void EntityInspectorWidget::Update()
{
	Dynarray<Entity*> selectedEntities = Manager->GetSelectedEntities();

	if (selectedEntities.GetSize() == 1)
	{
		std::stringstream ss;

		NameField->UpdateControl();

		ss << selectedEntities[0]->GetUUID();
		UniqueIdField->setText(&ss.str()[0]);

		// parent
		ss.str(std::string());
		if (selectedEntities[0]->IsRoot())
			ParentIdNameField->setText("");
		else
		{
			ss << selectedEntities[0]->GetParent()->GetName() + "\t<";
			ss << selectedEntities[0]->GetParent()->GetUUID();
			ss << "> ";
			ParentIdNameField->setText(&ss.str()[0]);
		}

		//children
		ChildrenIdNameField->clear();
		for (auto& child : selectedEntities[0]->GetChildren())
		{
			ss.str(std::string());
			ss << child->GetName() + "  <";
			ss << child->GetUUID();
			ss << "> ";
			ChildrenIdNameField->addItem(&ss.str()[0]);
		}

		if (!selectedEntities[0]->IsRoot())
			Transform->UpdateControl();
	}
	else if (selectedEntities.GetSize() > 1)
	{
		bool sameParent = true;
		Entity* parent = selectedEntities[0]->GetParent();

		for (Entity* e : selectedEntities)
			if (e->GetParent() != parent)
			{
				sameParent = false;
				break;
			}

		if (sameParent)
		{
			std::stringstream ss;
			ss.str(std::string());
			ss << selectedEntities[0]->GetParent()->GetName() + "\t<";
			ss << selectedEntities[0]->GetParent()->GetUUID();
			ss << "> ";
			ParentIdNameField->setText(&ss.str()[0]);
		}
		else
		{
			ParentIdNameField->setText("< multiple selection >");
		}
	}

	for (RTTIInspectorWidget* inspector : ComponentInspectors)
		inspector->UpdateInspector();
}

//------------------------------------------------------------------------------
void EntityInspectorWidget::Reload()
{
	EntitiesSelectionChanged();
}



//		internal
//------------------------------------------------------------------------------
void EntityInspectorWidget::ReloadComponentSections()
{
	// remove all old entity component sections
	for (auto cmp : ComponentSections)
	{
		MainLayout->removeWidget(cmp);
		delete cmp;
	}
	ComponentSections.Clear();
	ComponentInspectors.Clear();

	// add component sections
	for (size_t i = 0, row = 5; i < MAX_COMPONENTS_COUNT; ++i)
	{
		ComponentBase* cmp = Manager->GetSelectedEntities()[0]->GetComponent(i);
		if (!cmp) continue;

		cmp->GetTypeInfo();
		cmp->GetTypeInfo().GetTypeName();

		SectionContainer* section = new SectionContainer(cmp->GetTypeInfo().GetTypeName());
		RTTIInspectorWidget* viewer = new RTTIInspectorWidget(this);

		viewer->SetObject(cmp, false);
		connect(viewer, &RTTIInspectorWidget::ControlObjectUpdated,
			this, &EntityInspectorWidget::ControlObjectUpdated);

		ComponentSections.PushBack(section);
		ComponentInspectors.PushBack(viewer);

		section->SetWidget(viewer);
		MainLayout->addWidget(section, (int)row, 0, 1, 3);
		++row;
	}
}

//------------------------------------------------------------------------------
void EntityInspectorWidget::SpawnContextMenu(QPoint pos)
{
	if (Manager->GetSelectedEntities().GetSize() == 1)
		ContextMenu->popup(this->mapToGlobal(pos));
}



//		functions modifying state (should emit events)
//------------------------------------------------------------------------------
void EntityInspectorWidget::ControlObjectUpdated(Command* c)
{
	Manager->EntitiesModifiedSlot(c);
}

//------------------------------------------------------------------------------
void EntityInspectorWidget::AddComponent()
{
	ComponentDialog dialog;
	dialog.AddComponents(Manager->GetSelectedEntities()[0]);

	// TODO(squares): return components ids
	if (!dialog.OperationCanceled())
		Manager->ComponentsAdded();
}

//------------------------------------------------------------------------------
void EntityInspectorWidget::RemoveComponent()
{
	ComponentDialog dialog;
	dialog.RemoveComponents(Manager->GetSelectedEntities()[0]);

	// TODO(squares): return components ids
	if (!dialog.OperationCanceled())
		Manager->ComponentsRemoved();
}

//------------------------------------------------------------------------------
void EntityInspectorWidget::SelectParent()
{
	Manager->EntitiesSelectionChangedSlot({ Manager->GetSelectedEntities()[0]->GetParent() });
}

//------------------------------------------------------------------------------
void EntityInspectorWidget::SelectChild()
{
	//Manager->EntitiesSelectionChangedSlot(
	//	{ Manager->GetSelectedEntities()[0]->GetChildren()[ChildrenIdNameField->currentIndex()] });
}
