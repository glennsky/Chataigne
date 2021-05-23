/*
  ==============================================================================

    CustomValuesCommandArgument.cpp
    Created: 22 Feb 2017 8:51:30am
    Author:  Ben

  ==============================================================================
*/

#include "CustomValuesCommandArgument.h"
#include "ui/CustomValuesCommandArgumentEditor.h"

CustomValuesCommandArgument::CustomValuesCommandArgument(const String& name, Parameter* _p, bool _mappingEnabled, bool templateMode, Multiplex* multiplex, bool enablePrecison) :
	BaseItem(name, false),
	MultiplexTarget(multiplex),
	param(_p),
	editable(nullptr),
	sendPrecision(nullptr),
    mappingEnabled(_mappingEnabled),
	templateMode(templateMode),
	linkedTemplate(nullptr),
	enablePrecison(enablePrecison)
{
	editorCanBeCollapsed = false;

	isSelectable = false;
	
	jassert(param != nullptr);
	param->isSavable = false; // save manually
	addControllable(param);

	if(templateMode) param->isCustomizableByUser = true;
	
	param->forceSaveValue = true;
	param->saveValueOnly = !templateMode;
	param->forceSaveRange = true;
	
	if (templateMode)
	{
		editable = addBoolParameter("Editable", "If unchecked, this parameter will not be editable when instantiating this template command", true);
		editable->hideInEditor = true;
	}

	if (enablePrecison && param->type == Controllable::INT)
	{
		sendPrecision = addEnumParameter("Precision", "Type of int sent to the device, determine the number of bytes sent for this variable", true);
		sendPrecision->addOption("4 bytes (Int32)", IntType:: INT32)->addOption("2 bytes (Int16)", IntType::INT16)->addOption("1 byte", IntType::BYTE);
		//sendPrecision->addOption("uInt32", IntType::UINT32)->addOption("uInt16", IntType::UINT16)->addOption("Char (signed Byte)", IntType::CHAR);
		sendPrecision->hideInEditor = true;
	}

	//argumentName = addStringParameter("Argument name", "Name for the argument", "Arg");

	if (mappingEnabled || isMultiplexed())
	{
		paramLink.reset(new ParameterLink(param, multiplex));
	}

	param->hideInEditor = true;
}

CustomValuesCommandArgument::~CustomValuesCommandArgument()
{
	linkToTemplate(nullptr);
}


var CustomValuesCommandArgument::getJSONData()
{
	var data = BaseItem::getJSONData();
	data.getDynamicObject()->setProperty("param", param->getJSONData());
	if(paramLink != nullptr) data.getDynamicObject()->setProperty("paramLink", paramLink->getJSONData());
	return data;
}

void CustomValuesCommandArgument::loadJSONDataInternal(var data)
{
	BaseItem::loadJSONDataInternal(data);
	param->loadJSONData(data.getProperty("param", var()));
	if(paramLink != nullptr) paramLink->loadJSONData(data.getProperty("paramLink", var()));

}

void CustomValuesCommandArgument::linkToTemplate(CustomValuesCommandArgument * t)
{
	if (linkedTemplate != nullptr && !linkedTemplateRef.wasObjectDeleted())
	{
		linkedTemplate->param->removeParameterListener(this);
		linkedTemplate->editable->removeParameterListener(this);
		linkedTemplate->sendPrecision->removeParameterListener(this);
		linkedTemplate = nullptr;
	}

	linkedTemplate = t;

	if (linkedTemplate != nullptr)
	{
		linkedTemplate->param->addParameterListener(this);
		linkedTemplate->editable->addParameterListener(this);
		linkedTemplate->sendPrecision->addParameterListener(this);
		if (!templateMode) updateParameterFromTemplate();
	}
	

	if (param != nullptr)
	{
		param->saveValueOnly = linkedTemplate != nullptr;
		param->isCustomizableByUser = linkedTemplate == nullptr;
	}

	canBeReorderedInEditor = linkedTemplate == nullptr;
	userCanRemove = linkedTemplate == nullptr;
	userCanDuplicate = linkedTemplate == nullptr;
	nameCanBeChangedByUser = linkedTemplate == nullptr;


}

void CustomValuesCommandArgument::updateParameterFromTemplate()
{
	if (linkedTemplate != nullptr)
	{
		bool editEnabled = !linkedTemplate->editable->boolValue();
		param->setControllableFeedbackOnly(editEnabled);
		if(sendPrecision != nullptr) sendPrecision->setControllableFeedbackOnly(editEnabled);
		if(linkedTemplate->param->hasRange()) param->setRange(linkedTemplate->param->minimumValue, linkedTemplate->param->maximumValue);

		if (paramLink != nullptr && linkedTemplate->paramLink != nullptr) paramLink->loadJSONData(linkedTemplate->paramLink->getJSONData());

		//if (useForMapping != nullptr && linkedTemplate->useForMapping != nullptr && !useForMapping->isOverriden) useForMapping->setValue(linkedTemplate->useForMapping->boolValue());
		
		param->defaultValue = linkedTemplate->param->value;

		if (param->isControllableFeedbackOnly || !param->isOverriden)
		{
			if (param->type == Controllable::ENUM)
			{
				EnumParameter * lep = (EnumParameter *)linkedTemplate->param;
				EnumParameter * ep = (EnumParameter *)param;
				ep->clearOptions();
				for (auto &ev : lep->enumValues) ep->addOption(ev->key, ev->value);
			}
			
			param->resetValue();
		}
	}
}

void CustomValuesCommandArgument::onExternalParameterValueChanged(Parameter * p)
{
	if (p->parentContainer == linkedTemplate) updateParameterFromTemplate();
}

void CustomValuesCommandArgument::onExternalParameterRangeChanged(Parameter* p)
{
	if (p->parentContainer == linkedTemplate) updateParameterFromTemplate();
}

var CustomValuesCommandArgument::getLinkedValue(int multiplexIndex)
{
	if (paramLink == nullptr) return param->getValue();
	return paramLink->getLinkedValue(multiplexIndex);
}

String CustomValuesCommandArgument::getTypeString() const
{
	return param->getTypeString();
}


InspectableEditor * CustomValuesCommandArgument::getEditor(bool isRoot)
{
	return new CustomValuesCommandArgumentEditor(this, isRoot);
}