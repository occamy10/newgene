#include "../UIActionManager.h"

#ifndef Q_MOC_RUN
	#include <boost/scope_exit.hpp>
#endif
#include "../../Project/InputProject.h"
#include "../../Project/OutputProject.h"
#include "../../UIData/DataChanges.h"
#include "../../UIAction/ActionChanges.h"
#include "../../Utilities/TimeRangeHelper.h"
#include "../../Project/ProjectManager.h"

/************************************************************************/
// ACTION_CREATE_VG
// ACTION_DELETE_VG
// ACTION_SET_VG_DESCRIPTIONS
// ACTION_REFRESH_VG
/************************************************************************/

void UIActionManager::CreateVG(Messager & messager__, WidgetActionItemRequest_ACTION_CREATE_VG const & action_request, InputProject & project)
{

	if (FailIfBusy(messager__))
	{
		return;
	}

	BOOST_SCOPE_EXIT(this_)
	{
		this_->EndFailIfBusy();
	} BOOST_SCOPE_EXIT_END

	if (!action_request.items)
	{
		boost::format msg("There are no new VGs to add.");
		messager__.ShowMessageBox(msg.str());
		return;
	}

	InputModel & input_model = project.model();

	if (!action_request.items || action_request.items->size() == 0)
	{
		boost::format msg("There are no new VGs to add.");
		messager__.ShowMessageBox(msg.str());
		return;
	}

	switch (action_request.reason)
	{

		case WIDGET_ACTION_ITEM_REQUEST_REASON__ADD_ITEMS:
			{

				DataChangeMessage change_response(&project);

				for_each(action_request.items->cbegin(), action_request.items->cend(), [&input_model, &messager__, &change_response](InstanceActionItem const & instanceActionItem)
				{

					Executor executor(input_model.getDb());

					if (!instanceActionItem.second)
					{
						boost::format msg("Missing the new VG to create.");
						messager__.ShowMessageBox(msg.str());
						return;
					}

					// ************************************* //
					// Retrieve data sent by user interface
					// ************************************* //
					WidgetInstanceIdentifier const & uoa_to_use = instanceActionItem.first;
					WidgetActionItem const & actionItem = *instanceActionItem.second;
					WidgetActionItem__StringVector const & actionItemStrings = static_cast<WidgetActionItem__StringVector const &>(actionItem);
					std::vector<std::string> const & vg_strings = actionItemStrings.getValue();

					if (vg_strings.size() < 3)
					{
						boost::format msg("Incorrect internal data format for VG creation.");
						messager__.ShowMessageBox(msg.str());
						return;
					}

					size_t i = 0;
					std::string const & new_vg_code = vg_strings[i++];
					std::string const & vg_description = vg_strings[i++];
					std::string const & vg_longdescription = vg_strings[i++];

					bool vg_already_exists = input_model.t_vgp_identifiers.ExistsByCode(input_model.getDb(), input_model, new_vg_code);

					if (vg_already_exists)
					{
						boost::format msg("The VG code '%1%' already exists.");
						msg % boost::to_upper_copy(new_vg_code);
						messager__.ShowMessageBox(msg.str());
						return;
					}

					bool vg_successfully_created = input_model.t_vgp_identifiers.CreateNewVG(input_model.getDb(), input_model, new_vg_code, vg_description, vg_longdescription, uoa_to_use);

					if (!vg_successfully_created)
					{
						boost::format msg("Unable to execute INSERT statement to create a new VG category.");
						throw NewGeneException() << newgene_error_description(msg.str());
					}

					input_model.t_vgp_identifiers.Load(input_model.getDb(), &input_model); // re-sorts

					// ***************************************** //
					// Prepare data to send back to user interface
					// ***************************************** //

					WidgetInstanceIdentifier newIdentifier;
					bool found_newly_created_vg = input_model.t_vgp_identifiers.getIdentifierFromStringCode(new_vg_code, newIdentifier);

					if (!found_newly_created_vg || !newIdentifier.uuid || newIdentifier.uuid->empty() || !newIdentifier.code || newIdentifier.code->empty() || !newIdentifier.identifier_parent)
					{
						boost::format msg("Unable to find newly created VG.");
						throw NewGeneException() << newgene_error_description(msg.str());
					}

					DATA_CHANGE_TYPE type = DATA_CHANGE_TYPE__INPUT_MODEL__VG_CHANGE;
					DATA_CHANGE_INTENTION intention = DATA_CHANGE_INTENTION__ADD;
					DataChange change(type, intention, newIdentifier, WidgetInstanceIdentifiers());

					change_response.changes.push_back(change);

					executor.success();

				});

				messager__.EmitChangeMessage(change_response);

			}
			break;

		default:
			break;

	}

}

void UIActionManager::DeleteVG(Messager & messager, WidgetActionItemRequest_ACTION_DELETE_VG const & action_request, InputProject & project)
{

	if (!action_request.items)
	{
		return;
	}

	InputModel & input_model = project.model();

	switch (action_request.reason)
	{

		case WIDGET_ACTION_ITEM_REQUEST_REASON__REMOVE_ITEMS:
			{

				DataChangeMessage change_response(&project);

				std::for_each(action_request.items->cbegin(), action_request.items->cend(), [this, &input_model, &messager, &change_response](InstanceActionItem const & instanceActionItem)
				{

					ProjectManager & project_manager = projectManager();
					std::string errorMsg;
					bool proceed = project_manager.LetMeRunTask(ProjectManager::PROJECT_TYPE__INPUT, instanceActionItem.second->id, std::string("delete_vg"), errorMsg);

					if (!proceed)
					{
						boost::format msg("Error deleting variable group: %1%");
						msg % errorMsg.c_str();
						messager.ShowMessageBox(msg.str());
						return;
					}

					BOOST_SCOPE_EXIT_ALL(&)
					{
						bool success = project_manager.TaskCompleted(ProjectManager::PROJECT_TYPE__INPUT, instanceActionItem.second->id, std::string("delete_vg"), errorMsg);

						if (!success)
						{
							boost::format msg("Error deleting variable group: %1%. Please restart NewGene.");
							msg % errorMsg.c_str();
							messager.ShowMessageBox(msg.str());
						}
					};

					if (this->FailIfBusy(messager))
					{
						return;
					}

					BOOST_SCOPE_EXIT_ALL(&, this)
					{
						this->EndFailIfBusy();
					};

					Executor executor(input_model.getDb());

					WidgetInstanceIdentifier vg = instanceActionItem.first;

					if (!vg.uuid || vg.uuid->empty() || !vg.code || vg.code->empty())
					{
						boost::format msg("Missing the VG to delete.");
						messager.ShowMessageBox(msg.str());
						return;
					}

					// ************************************* //
					// Retrieve data sent by user interface
					// ************************************* //
					std::string vg_to_delete_display_text = Table_VG_CATEGORY::GetVgDisplayText(vg);

					bool vg_already_exists = input_model.t_vgp_identifiers.Exists(input_model.getDb(), input_model, vg);

					if (!vg_already_exists)
					{
						boost::format msg("The VG '%1%' is already absent.");
						msg % boost::to_upper_copy(vg_to_delete_display_text);
						messager.ShowMessageBox(msg.str());
						return;
					}

					bool vg_successfully_deleted = input_model.t_vgp_identifiers.DeleteVG(input_model.getDb(), &input_model, vg, change_response);

					if (!vg_successfully_deleted)
					{
						boost::format msg("Unable to delete the VG.");
						throw NewGeneException() << newgene_error_description(msg.str());
					}

					executor.success();

				});

				messager.EmitChangeMessage(change_response);

			}
			break;

		default:
			break;

	}

}

void UIActionManager::SetVGDescriptions(Messager & messager, WidgetActionItemRequest_ACTION_SET_VG_DESCRIPTIONS const & action_request, InputProject & project)
{

	if (!action_request.items)
	{
		return;
	}

	InputModel & input_model = project.model();

	switch (action_request.reason)
	{

		case WIDGET_ACTION_ITEM_REQUEST_REASON__UPDATE_ITEMS:
			{

				DataChangeMessage change_response(&project);

				std::for_each(action_request.items->cbegin(), action_request.items->cend(), [this, &input_model, &messager, &change_response](InstanceActionItem const & instanceActionItem)
				{

					ProjectManager & project_manager = projectManager();
					std::string errorMsg;
					bool proceed = project_manager.LetMeRunTask(ProjectManager::PROJECT_TYPE__INPUT, instanceActionItem.second->id, std::string("set_descriptions_for_vg"), errorMsg);

					if (!proceed)
					{
						boost::format msg("Error renaming variable group: %1%");
						msg % errorMsg.c_str();
						messager.ShowMessageBox(msg.str());
						return;
					}

					BOOST_SCOPE_EXIT_ALL(&)
					{
						bool success = project_manager.TaskCompleted(ProjectManager::PROJECT_TYPE__INPUT, instanceActionItem.second->id, std::string("set_descriptions_for_vg"), errorMsg);

						if (!success)
						{
							boost::format msg("Error renaming variable group: %1%. Please restart NewGene.");
							msg % errorMsg.c_str();
							messager.ShowMessageBox(msg.str());
						}
					};

					if (this->FailIfBusy(messager))
					{
						return;
					}

					BOOST_SCOPE_EXIT_ALL(&, this)
					{
						this->EndFailIfBusy();
					};

					Executor executor(input_model.getDb());

					WidgetInstanceIdentifier vg = instanceActionItem.first;

					if (!vg.uuid || vg.uuid->empty() || !vg.code || vg.code->empty())
					{
						boost::format msg("Missing the VG whose descriptions are to be changed.");
						messager.ShowMessageBox(msg.str());
						return;
					}

					// ************************************* //
					// Retrieve data sent by user interface
					// ************************************* //
					std::string vg_to_set_descriptions_display_text = Table_VG_CATEGORY::GetVgDisplayText(vg);
					std::vector<std::string> vgNewMetadata = (static_cast<WidgetActionItem__StringVector const &>(*instanceActionItem.second)).getValue();

					if (vgNewMetadata.size() < 2)
					{
						boost::format msg("Insufficient number of parameters for the VG metadata update operation.");
						messager.ShowMessageBox(msg.str());
						return;
					}

					std::string vg_new_description = vgNewMetadata[0];
					std::string vg_new_longdescription = vgNewMetadata[1];

					bool vg_already_exists = input_model.t_vgp_identifiers.Exists(input_model.getDb(), input_model, vg);

					if (!vg_already_exists)
					{
						boost::format msg("The VG '%1%' does not exist.");
						msg % boost::to_upper_copy(vg_to_set_descriptions_display_text);
						messager.ShowMessageBox(msg.str());
						return;
					}

					bool vg_successfully_set = input_model.t_vgp_identifiers.SetVGDescriptions(input_model.getDb(), &input_model, vg, vg_new_description, vg_new_longdescription, change_response);

					if (!vg_successfully_set)
					{
						boost::format msg("Unable to set descriptions for the VG.");
						throw NewGeneException() << newgene_error_description(msg.str());
					}

					input_model.t_vgp_identifiers.Load(input_model.getDb(), &input_model); // re-sorts

					executor.success();

				});

				messager.EmitChangeMessage(change_response);

			}
			break;

		default:
			break;

	}

}

void UIActionManager::DeleteVGOutput(Messager & messager, WidgetActionItemRequest_ACTION_DELETE_VG const & action_request, OutputProject & project)
{

	if (!action_request.items)
	{
		return;
	}

	OutputModel & output_model = project.model();
	InputModel & input_model = output_model.getInputModel();

	switch (action_request.reason)
	{

		case WIDGET_ACTION_ITEM_REQUEST_REASON__REMOVE_ITEMS:
			{

				DataChangeMessage change_response(&project);

				for_each(action_request.items->cbegin(), action_request.items->cend(), [this, &output_model, &input_model, &messager,
						 &change_response](InstanceActionItem const & instanceActionItem)
				{

					ProjectManager & project_manager = projectManager();
					std::string errorMsg;
					bool proceed = project_manager.LetMeRunTask(ProjectManager::PROJECT_TYPE__OUTPUT, instanceActionItem.second->id, std::string("delete_vg"), errorMsg);

					if (!proceed)
					{
						boost::format msg("Error deleting variable group: %1%");
						msg % errorMsg.c_str();
						messager.ShowMessageBox(msg.str());
						return;
					}

					BOOST_SCOPE_EXIT_ALL(&)
					{
						bool success = project_manager.TaskCompleted(ProjectManager::PROJECT_TYPE__OUTPUT, instanceActionItem.second->id, std::string("delete_vg"), errorMsg);

						if (!success)
						{
							boost::format msg("Error deleting variable group: %1%. Please restart NewGene.");
							msg % errorMsg.c_str();
							messager.ShowMessageBox(msg.str());
						}
					};

					if (this->FailIfBusy(messager))
					{
						return;
					}

					BOOST_SCOPE_EXIT_ALL(&, this)
					{
						this->EndFailIfBusy();
					};

					Executor executor(input_model.getDb());

					WidgetInstanceIdentifier vg = instanceActionItem.first;

					if (!vg.uuid || vg.uuid->empty() || !vg.code || vg.code->empty())
					{
						// Error should already be handled in input model function
						return;
					}

					output_model.t_variables_selected_identifiers.RemoveAllfromVG(output_model.getDb(), vg);

					// ***************************************** //
					// Prepare data to send back to user interface
					// ***************************************** //
					DATA_CHANGE_TYPE type = DATA_CHANGE_TYPE__OUTPUT_MODEL__ACTIVE_DMU_CHANGE;
					DATA_CHANGE_INTENTION intention = DATA_CHANGE_INTENTION__UPDATE;
					DataChange change(type, intention, WidgetInstanceIdentifier(), WidgetInstanceIdentifiers());
					change_response.changes.push_back(change);

					// ***************************************** //
					// Use updated cache info to set further info
					// in change_response
					// ***************************************** //
					std::set<WidgetInstanceIdentifier> active_dmus = output_model.t_variables_selected_identifiers.GetActiveDMUs(&output_model, &input_model);
					change_response.changes.back().set_of_identifiers = active_dmus;

					executor.success();

				});

				messager.EmitChangeMessage(change_response);

			}
			break;

		default:
			break;

	}

}

void UIActionManager::SetVGDescriptionsOutput(Messager & messager, WidgetActionItemRequest_ACTION_SET_VG_DESCRIPTIONS const & action_request, OutputProject & project)
{

	if (!action_request.items)
	{
		return;
	}

	OutputModel & output_model = project.model();
	InputModel & input_model = output_model.getInputModel();

	switch (action_request.reason)
	{

		case WIDGET_ACTION_ITEM_REQUEST_REASON__UPDATE_ITEMS:
			{

				DataChangeMessage change_response(&project);

				for_each(action_request.items->cbegin(), action_request.items->cend(), [this, &output_model, &input_model, &messager,
						 &change_response](InstanceActionItem const & instanceActionItem)
				{

					ProjectManager & project_manager = projectManager();
					std::string errorMsg;
					bool proceed = project_manager.LetMeRunTask(ProjectManager::PROJECT_TYPE__OUTPUT, instanceActionItem.second->id, std::string("set_descriptions_for_vg"), errorMsg);

					if (!proceed)
					{
						boost::format msg("Error renaming variable group: %1%");
						msg % errorMsg.c_str();
						messager.ShowMessageBox(msg.str());
						return;
					}

					BOOST_SCOPE_EXIT_ALL(&)
					{
						bool success = project_manager.TaskCompleted(ProjectManager::PROJECT_TYPE__OUTPUT, instanceActionItem.second->id, std::string("set_descriptions_for_vg"), errorMsg);

						if (!success)
						{
							boost::format msg("Error renaming variable group: %1%. Please restart NewGene.");
							msg % errorMsg.c_str();
							messager.ShowMessageBox(msg.str());
						}
					};

					if (this->FailIfBusy(messager))
					{
						return;
					}

					BOOST_SCOPE_EXIT_ALL(&, this)
					{
						this->EndFailIfBusy();
					};

					Executor executor(input_model.getDb());

					WidgetInstanceIdentifier vg = instanceActionItem.first;
					std::vector<std::string> vgDescriptions = (static_cast<WidgetActionItem__StringVector const &>(*instanceActionItem.second)).getValue();

					if (vgDescriptions.size() < 2)
					{
						return;
					}

					std::string vg_new_description = vgDescriptions[0];
					std::string vg_new_longdescription = vgDescriptions[1];

					if (!vg.uuid || vg.uuid->empty() || !vg.code || vg.code->empty())
					{
						// Error should already be handled in input model function
						return;
					}

					output_model.t_variables_selected_identifiers.SetDescriptionsAllInVG(output_model.getDb(), vg, vg_new_description, vg_new_longdescription);

					executor.success();

				});

				messager.EmitChangeMessage(change_response);

			}
			break;

		default:
			break;

	}

}

void UIActionManager::RefreshVG(Messager & messager, WidgetActionItemRequest_ACTION_REFRESH_VG const & action_request, InputProject & project)
{

	if (FailIfBusy(messager))
	{
		return;
	}

	BOOST_SCOPE_EXIT(this_)
	{
		this_->EndFailIfBusy();
	} BOOST_SCOPE_EXIT_END

	if (!action_request.items)
	{
		return;
	}

	{
		std::lock_guard<std::recursive_mutex> guard(Importer::is_performing_import_mutex);

		if (Importer::is_performing_import)
		{
			boost::format msg("Another import operation is in progress.  Please wait for that operation to complete first.");
			throw NewGeneException() << newgene_error_description(msg.str());
		}
	}

	InputModel & input_model = project.model();

	switch (action_request.reason)
	{

		case WIDGET_ACTION_ITEM_REQUEST_REASON__DO_ACTION:
			{

				DataChangeMessage change_response(&project);

				for_each(action_request.items->cbegin(), action_request.items->cend(), [this, &input_model, &messager, &change_response](InstanceActionItem const & instanceActionItem)
				{

					if (!instanceActionItem.second)
					{
						boost::format msg("Missing VG refresh information.");
						messager.ShowMessageBox(msg.str());
						return;
					}

					// ************************************* //
					// Retrieve data sent by user interface
					// ************************************* //
					WidgetActionItem const & actionItem = *instanceActionItem.second;
					WidgetActionItem__ImportVariableGroup const & actionItemData = static_cast<WidgetActionItem__ImportVariableGroup const &>(actionItem);

					WidgetInstanceIdentifier variable_group = actionItemData.getVG();
					std::vector<std::string> timeRangeColumnNames = actionItemData.getTimeRangeColNames();
					std::vector<std::pair<WidgetInstanceIdentifier, std::string>> dmusAndColumnNames = actionItemData.getDmusAndColNames();
					boost::filesystem::path filePathName = actionItemData.getFilePathName();
					TIME_GRANULARITY time_granularity = actionItemData.getTimeGranularity();
					bool inputFileContainsColumnDescriptions = actionItemData.doesInputFileContainsColumnDescriptions();
					bool inputFileContainsColumnDataTypes = actionItemData.doesInputFileContainsColumnDataTypes();
					bool do_refresh_not_plain_insert = actionItemData.doRefreshNotPlainInsert();

					if (!variable_group.uuid || variable_group.uuid->empty() || !variable_group.code || variable_group.code->empty())
					{
						boost::format msg("Missing the VG to refresh.");
						messager.ShowMessageBox(msg.str());
						return;
					}

					std::string errorMsg;
					std::unique_ptr<Table_VariableGroupData> new_table(new Table_VariableGroupData(*variable_group.code));

					if (new_table == nullptr)
					{
						boost::format msg("Out of memory.  Cannot import the variable group data.");
						messager.ShowMessageBox(msg.str());
						return;
					}

					ImportDefinition import_definition;
					errorMsg.clear();
					bool success = new_table->BuildImportDefinition(input_model.getDb(), &input_model, variable_group, timeRangeColumnNames, dmusAndColumnNames, filePathName, time_granularity,
								   inputFileContainsColumnDescriptions, inputFileContainsColumnDataTypes, import_definition, errorMsg);

					if (!success)
					{
						new_table->DeleteDataTable(input_model.getDb(), &input_model);
						boost::format msg("Failed to build the import definition: %1%");
						msg % errorMsg;
						messager.ShowMessageBox(msg.str());
						return;
					}

					// Create the table importer
					errorMsg.clear();

					Importer::Mode import_mode;

					if (do_refresh_not_plain_insert)
					{
						import_mode = Importer::INSERT_OR_UPDATE;
					}
					else
					{
						import_mode = Importer::INSERT_IN_BULK;
					}

					Importer table_importer(import_definition, &input_model, new_table.get(), import_mode, variable_group, InputModelImportTableFn, Importer::IMPORT_VG_INSTANCE_DATA, errorMsg);

					if (!errorMsg.empty())
					{
						boost::format msg("Unable to create data table: %1%");
						msg % errorMsg;
						messager.ShowMessageBox(msg.str());
						return;
					}

					{

						Executor executor(input_model.getDb());

						// Add the data columns of the table to the VG_SET_MEMBER table
						errorMsg.clear();
						success = input_model.t_vgp_setmembers.AddNewVGTableEntries(input_model.getDb(), &input_model, variable_group, import_definition, errorMsg);

						if (!success)
						{
							new_table->DeleteDataTable(input_model.getDb(), &input_model);
							boost::format msg("%1%");

							if (!errorMsg.empty())
							{
								msg % errorMsg;
							}
							else
							{
								msg % "Unable to create metadata entries describing the columns for the variable group.";
							}

							messager.ShowMessageBox(msg.str());
							return;
						}

						// Add the metadata for the new table to the VG_DATA_METADATA__DATETIME_COLUMNS table
						errorMsg.clear();
						success = input_model.t_vgp_data_metadata__datetime_columns.AddDataTable(input_model.getDb(), &input_model, variable_group, errorMsg, time_granularity);

						if (!success)
						{
							new_table->DeleteDataTable(input_model.getDb(), &input_model);
							boost::format msg("%1%");

							if (!errorMsg.empty())
							{
								msg % errorMsg;
							}
							else
							{
								msg % "Unable to create date/time column entries for the variable group.";
							}

							messager.ShowMessageBox(msg.str());
							return;
						}

						// Add the metadata for the new table to the VG_DATA_METADATA__PRIMARY_KEYS table
						errorMsg.clear();
						success = input_model.t_vgp_data_metadata__primary_keys.AddDataTable(input_model.getDb(), &input_model, variable_group, import_definition.primary_keys_info, errorMsg);

						if (!success)
						{
							new_table->DeleteDataTable(input_model.getDb(), &input_model);

							if (!errorMsg.empty())
							{
								boost::format msg("%1%: %2%");
								msg % "Unable to create primary key metadata column entries for the variable group.";
								msg % errorMsg;
								messager.ShowMessageBox(msg.str());
							}
							else
							{
								boost::format msg("%1%");
								msg % "Unable to create primary key metadata column entries for the variable group.";
								messager.ShowMessageBox(msg.str());
							}

							return;
						}

						executor.success();

					}

					{

						// Now add raw data to the table
						errorMsg.clear();
						std::string msgBoxErrors;
						success = table_importer.DoImport(errorMsg, messager);

						if (table_importer.badreadlines > 0)
						{
							boost::format msg("Number rows of data failed to read from import file: %1%");
							msg % boost::lexical_cast<std::string>(table_importer.badreadlines);
							table_importer.errors.push_back(msg.str());
							msgBoxErrors += msg.str();
							msgBoxErrors += "\n";
						}

						if (table_importer.badwritelines > 0)
						{
							boost::format msg("Number rows of data failed to write to database: %1%");
							msg % boost::lexical_cast<std::string>(table_importer.badwritelines);
							table_importer.errors.push_back(msg.str());
							msgBoxErrors += msg.str();
							msgBoxErrors += "\n";
						}

						boost::posix_time::ptime current_date_time = boost::posix_time::second_clock::local_time();

						if (!success)
						{
							boost::format msg("%1%: Unable to import or refresh the variable group from the file: %2%");
							msg % boost::posix_time::to_simple_string(current_date_time).c_str() % errorMsg;
							std::string new_error(msg.str());
							table_importer.errors.push_back(new_error);
							msgBoxErrors += msg.str();
							msgBoxErrors += "\n";
						}

						if (!table_importer.errors.empty())
						{
							boost::format msg("%1%: There were messages during import.  These will be appended to log \"newgene.import.log\"");
							msg % boost::posix_time::to_simple_string(current_date_time).c_str();
							std::string errorMsg = msg.str();
							table_importer.errors.push_back(errorMsg);
							msgBoxErrors += msg.str();
							msgBoxErrors += "\n";

							std::fstream importlog;
							importlog.open(messager.GetSystemDependentPath(MESSAGER_PATH_ENUM__IMPORT_LOG).string(), std::ios::out | std::ios::app);
							std::for_each(table_importer.errors.crbegin(), table_importer.errors.crend(), [&](std::string const & the_error)
							{
								if (importlog.is_open())
								{
									importlog << the_error << std::endl;
								}
							});
							importlog.close();
							messager.ShowMessageBox(msgBoxErrors);
						}

						if (!success)
						{
							new_table->DeleteDataTable(input_model.getDb(), &input_model);
							boost::format msg("%1%");
							msg % msgBoxErrors;
							throw NewGeneException() << newgene_error_description(msg.str());
						}

						// Success!  Turn the pointer over to the input model
						input_model.t_vgp_data_vector.push_back(std::move(new_table));

						std::string cancelAddendum;

						if (Importer::cancelled)
						{
							cancelAddendum = " (until cancelled)";
						}

						if (table_importer.badreadlines > 0 || table_importer.badwritelines > 0)
						{
							if (table_importer.badreadlines > 0 && table_importer.badwritelines > 0)
							{
								if (do_refresh_not_plain_insert)
								{
									// Handle incoming data row-by-row, distinguishing between inserts and updates
									boost::format
									msg("Variable group '%1%' refreshed %5% rows from file%4% (%6% written to, %7% updated in database), but %2% rows failed when being read from the input file and %3% rows failed to be written to the database.  See the \"newgene.import.log\" file for details.");
									msg % Table_VG_CATEGORY::GetVgDisplayText(variable_group)
									% boost::lexical_cast<std::string>(table_importer.badreadlines)
									% boost::lexical_cast<std::string>(table_importer.badwritelines)
									% cancelAddendum
									% boost::lexical_cast<std::string>(table_importer.goodreadlines)
									% boost::lexical_cast<std::string>(table_importer.goodwritelines)
									% boost::lexical_cast<std::string>(table_importer.goodupdatelines);
									messager.ShowMessageBox(msg.str());
								}
								else
								{
									// Bulk INSERT OR REPLACE mode - we do not currently distinguish between inserts and updates
									boost::format
									msg("Variable group '%1%' refreshed %5% rows from file%4% (%6% written to and/or updated in database), but %2% rows failed when being read from the input file and %3% rows failed to be written to the database.  See the \"newgene.import.log\" file for details.");
									msg % Table_VG_CATEGORY::GetVgDisplayText(variable_group)
									% boost::lexical_cast<std::string>(table_importer.badreadlines)
									% boost::lexical_cast<std::string>(table_importer.badwritelines)
									% cancelAddendum
									% boost::lexical_cast<std::string>(table_importer.goodreadlines)
									% boost::lexical_cast<std::string>(table_importer.goodwritelines + table_importer.goodupdatelines);
									messager.ShowMessageBox(msg.str());
								}
							}
							else if (table_importer.badreadlines == 0 && table_importer.badwritelines > 0)
							{
								if (do_refresh_not_plain_insert)
								{
									// Handle incoming data row-by-row, distinguishing between inserts and updates
									boost::format
									msg("Variable group '%1%' refreshed %4% rows from file%3% (%5% written to, %6% updated in database), but %2% rows failed to be written to the database.  See the \"newgene.import.log\" file for details.");
									msg % Table_VG_CATEGORY::GetVgDisplayText(variable_group)
									% boost::lexical_cast<std::string>(table_importer.badwritelines)
									% cancelAddendum
									% boost::lexical_cast<std::string>(table_importer.goodreadlines)
									% boost::lexical_cast<std::string>(table_importer.goodwritelines)
									% boost::lexical_cast<std::string>(table_importer.goodupdatelines);
									messager.ShowMessageBox(msg.str());
								}
								else
								{
									// Bulk INSERT OR REPLACE mode - we do not currently distinguish between inserts and updates
									boost::format
									msg("Variable group '%1%' refreshed %4% rows from file%3% (%5% written to and/or updated in database), but %2% rows failed to be written to the database.  See the \"newgene.import.log\" file for details.");
									msg % Table_VG_CATEGORY::GetVgDisplayText(variable_group)
									% boost::lexical_cast<std::string>(table_importer.badwritelines)
									% cancelAddendum
									% boost::lexical_cast<std::string>(table_importer.goodreadlines)
									% boost::lexical_cast<std::string>(table_importer.goodwritelines + table_importer.goodupdatelines);
									messager.ShowMessageBox(msg.str());
								}
							}
							else if (table_importer.badreadlines > 0 && table_importer.badwritelines == 0)
							{
								if (do_refresh_not_plain_insert)
								{
									// Handle incoming data row-by-row, distinguishing between inserts and updates
									boost::format
									msg("Variable group '%1%' refreshed %4% rows from from file%3% (%5% written to, %6% updated in database), but %2% rows failed when being read from the input file.  See the \"newgene.import.log\" file for details.");
									msg % Table_VG_CATEGORY::GetVgDisplayText(variable_group)
									% boost::lexical_cast<std::string>(table_importer.badreadlines)
									% cancelAddendum
									% boost::lexical_cast<std::string>(table_importer.goodreadlines)
									% boost::lexical_cast<std::string>(table_importer.goodwritelines)
									% boost::lexical_cast<std::string>(table_importer.goodupdatelines);
									messager.ShowMessageBox(msg.str());
								}
								else
								{
									// Bulk INSERT OR REPLACE mode - we do not currently distinguish between inserts and updates
									boost::format
									msg("Variable group '%1%' refreshed %4% rows from from file%3% (%5% written to and/or updated in database), but %2% rows failed when being read from the input file.  See the \"newgene.import.log\" file for details.");
									msg % Table_VG_CATEGORY::GetVgDisplayText(variable_group)
									% boost::lexical_cast<std::string>(table_importer.badreadlines)
									% cancelAddendum
									% boost::lexical_cast<std::string>(table_importer.goodreadlines)
									% boost::lexical_cast<std::string>(table_importer.goodwritelines + table_importer.goodupdatelines);
									messager.ShowMessageBox(msg.str());
								}
							}
						}
						else
						{
							if (!table_importer.CheckCancelled() && (table_importer.goodreadlines != table_importer.goodwritelines + table_importer.goodupdatelines))
							{
								boost::format
								msg("During refresh of variable group, although there were no read or write failures, nonetheless the number of successful lines read from input file (%1%) does not match the number of successful lines written to (%2%) and updated in (%3%) the database.");
								msg % boost::lexical_cast<std::string>(table_importer.goodreadlines) % boost::lexical_cast<std::string>(table_importer.goodwritelines) % boost::lexical_cast<std::string>
								(table_importer.goodupdatelines);
								throw NewGeneException() << newgene_error_description(msg.str());
							}

							if (do_refresh_not_plain_insert)
							{
								// Handle incoming data row-by-row, distinguishing between inserts and updates
								boost::format msg("Variable group '%1%' successfully read %3% rows from file (%4% updated and %5% inserted)%2%.");
								msg % Table_VG_CATEGORY::GetVgDisplayText(variable_group)
								% cancelAddendum
								% table_importer.goodreadlines
								% table_importer.goodupdatelines
								% table_importer.goodwritelines;
								messager.ShowMessageBox(msg.str());
							}
							else
							{
								// Bulk INSERT OR REPLACE mode - we do not currently distinguish between inserts and updates
								boost::format msg("Variable group '%1%' successfully read %3% rows from file (%4% written to database)%2%.");
								msg % Table_VG_CATEGORY::GetVgDisplayText(variable_group)
								% cancelAddendum
								% boost::lexical_cast<std::string>(table_importer.goodreadlines)
								% boost::lexical_cast<std::string>(table_importer.goodupdatelines + table_importer.goodwritelines);
								messager.ShowMessageBox(msg.str());
							}
						}

					}

					// ***************************************** //
					// Prepare data to send back to user interface
					// ***************************************** //

					WidgetInstanceIdentifiers vg_members = input_model.t_vgp_setmembers.getIdentifiers(*variable_group.uuid);

					DATA_CHANGE_TYPE type = DATA_CHANGE_TYPE__INPUT_MODEL__VG_CHANGE;
					DATA_CHANGE_INTENTION intention = DATA_CHANGE_INTENTION__UPDATE;
					DataChange change(type, intention, variable_group, vg_members);
					change_response.changes.push_back(change);

				});

				messager.EmitChangeMessage(change_response);

			}
			break;

		default:
			break;

	}

}
