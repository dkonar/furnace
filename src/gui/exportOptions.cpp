/**
 * Furnace Tracker - multi-system chiptune tracker
 * Copyright (C) 2021-2023 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "gui.h"
#include "guiConst.h"
#include "../fileutils.h"
#include "misc/cpp/imgui_stdlib.h"
#include <imgui.h>

void FurnaceGUI::drawExportAudio(bool onWindow) {
  exitDisabledTimer=1;

  ImGui::RadioButton("one file",&audioExportType,0);
  ImGui::RadioButton("multiple files (one per chip)",&audioExportType,1);
  ImGui::RadioButton("multiple files (one per channel)",&audioExportType,2);
  if (ImGui::InputInt("Loops",&exportLoops,1,2)) {
    if (exportLoops<0) exportLoops=0;
  }
  if (ImGui::InputDouble("Fade out (seconds)",&exportFadeOut,1.0,2.0,"%.1f")) {
    if (exportFadeOut<0.0) exportFadeOut=0.0;
  }

  if (onWindow) {
    ImGui::Separator();
    if (ImGui::Button("Cancel",ImVec2(200.0f*dpiScale,0))) ImGui::CloseCurrentPopup();
    ImGui::SameLine();
  }

  if (ImGui::Button("Export",ImVec2(200.0f*dpiScale,0))) {
    switch (audioExportType) {
      case 0:
        openFileDialog(GUI_FILE_EXPORT_AUDIO_ONE);
        break;
      case 1:
        openFileDialog(GUI_FILE_EXPORT_AUDIO_PER_SYS);
        break;
      case 2:
        openFileDialog(GUI_FILE_EXPORT_AUDIO_PER_CHANNEL);
        break;
    }
    ImGui::CloseCurrentPopup();
  }
}

void FurnaceGUI::drawExportVGM(bool onWindow) {
  exitDisabledTimer=1;

  ImGui::Text("settings:");
  if (ImGui::BeginCombo("format version",fmt::sprintf("%d.%.2x",vgmExportVersion>>8,vgmExportVersion&0xff).c_str())) {
    for (int i=0; i<7; i++) {
      if (ImGui::Selectable(fmt::sprintf("%d.%.2x",vgmVersions[i]>>8,vgmVersions[i]&0xff).c_str(),vgmExportVersion==vgmVersions[i])) {
        vgmExportVersion=vgmVersions[i];
      }
    }
    ImGui::EndCombo();
  }
  ImGui::Checkbox("loop",&vgmExportLoop);
  if (vgmExportLoop && e->song.loopModality==2) {
    ImGui::Text("loop trail:");
    ImGui::Indent();
    if (ImGui::RadioButton("auto-detect",vgmExportTrailingTicks==-1)) {
      vgmExportTrailingTicks=-1;
    }
    if (ImGui::RadioButton("add one loop",vgmExportTrailingTicks==-2)) {
      vgmExportTrailingTicks=-2;
    }
    if (ImGui::RadioButton("custom",vgmExportTrailingTicks>=0)) {
      vgmExportTrailingTicks=0;
    }
    if (vgmExportTrailingTicks>=0) {
      ImGui::SameLine();
      if (ImGui::InputInt("##TrailTicks",&vgmExportTrailingTicks,1,100)) {
        if (vgmExportTrailingTicks<0) vgmExportTrailingTicks=0;
      }
    }
    ImGui::Unindent();
  }
  ImGui::Checkbox("add pattern change hints",&vgmExportPatternHints);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
      "inserts data blocks on pattern changes.\n"
      "useful if you are writing a playback routine.\n\n"

      "the format of a pattern change data block is:\n"
      "67 66 FE ll ll ll ll 01 oo rr pp pp pp ...\n"
      "- ll: length, a 32-bit little-endian number\n"
      "- oo: order\n"
      "- rr: initial row (a 0Dxx effect is able to select a different row)\n"
      "- pp: pattern index (one per channel)\n\n"

      "pattern indexes are ordered as they appear in the song."
    );
  }
  ImGui::Checkbox("direct stream mode",&vgmExportDirectStream);
  if (ImGui::IsItemHovered()) {
    ImGui::SetTooltip(
      "required for DualPCM and MSM6258 export.\n\n"
      "allows for volume/direction changes when playing samples,\n"
      "at the cost of a massive increase in file size."
    );
  }
  ImGui::Text("chips to export:");
  bool hasOneAtLeast=false;
  for (int i=0; i<e->song.systemLen; i++) {
    int minVersion=e->minVGMVersion(e->song.system[i]);
    ImGui::BeginDisabled(minVersion>vgmExportVersion || minVersion==0);
    ImGui::Checkbox(fmt::sprintf("%d. %s##_SYSV%d",i+1,getSystemName(e->song.system[i]),i).c_str(),&willExport[i]);
    ImGui::EndDisabled();
    if (minVersion>vgmExportVersion) {
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("this chip is only available in VGM %d.%.2x and higher!",minVersion>>8,minVersion&0xff);
      }
    } else if (minVersion==0) {
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("this chip is not supported by the VGM format!");
      }
    } else {
      if (willExport[i]) hasOneAtLeast=true;
    }
  }
  ImGui::Text("select the chip you wish to export, but only up to %d of each type.",(vgmExportVersion>=0x151)?2:1);
  if (hasOneAtLeast) {
    if (onWindow) {
      ImGui::Separator();
      if (ImGui::Button("Cancel",ImVec2(200.0f*dpiScale,0))) ImGui::CloseCurrentPopup();
      ImGui::SameLine();
    }
    if (ImGui::Button("Export",ImVec2(200.0f*dpiScale,0))) {
      openFileDialog(GUI_FILE_EXPORT_VGM);
      ImGui::CloseCurrentPopup();
    }
  } else {
    ImGui::Text("nothing to export");
    if (onWindow) {
      ImGui::Separator();
      if (ImGui::Button("Cancel",ImVec2(400.0f*dpiScale,0))) ImGui::CloseCurrentPopup();
    }
  }
}

void FurnaceGUI::drawExportZSM(bool onWindow) {
  exitDisabledTimer=1;

  ImGui::Text("Commander X16 Zsound Music File");
  if (ImGui::InputInt("Tick Rate (Hz)",&zsmExportTickRate,1,2)) {
    if (zsmExportTickRate<1) zsmExportTickRate=1;
    if (zsmExportTickRate>44100) zsmExportTickRate=44100;
  }
  ImGui::Checkbox("loop",&zsmExportLoop);
  ImGui::SameLine();
  ImGui::Checkbox("optimize size",&zsmExportOptimize);
  if (onWindow) {
    ImGui::Separator();
    if (ImGui::Button("Cancel",ImVec2(200.0f*dpiScale,0))) ImGui::CloseCurrentPopup();
    ImGui::SameLine();
  }
  if (ImGui::Button("Export",ImVec2(200.0f*dpiScale,0))) {
    openFileDialog(GUI_FILE_EXPORT_ZSM);
    ImGui::CloseCurrentPopup();
  }
}

void FurnaceGUI::drawExportAmigaVal(bool onWindow) {
  exitDisabledTimer=1;

  ImGui::Text(
    "this is NOT ROM export! only use for making sure the\n"
    "Furnace Amiga emulator is working properly by\n"
    "comparing it with real Amiga output."
  );
  ImGui::AlignTextToFramePadding();
  ImGui::Text("Directory");
  ImGui::SameLine();
  ImGui::InputText("##AVDPath",&workingDirROMExport);
  if (onWindow) {
    ImGui::Separator();
    if (ImGui::Button("Cancel",ImVec2(200.0f*dpiScale,0))) ImGui::CloseCurrentPopup();
    ImGui::SameLine();
  }
  if (ImGui::Button("Bake Data",ImVec2(200.0f*dpiScale,0))) {
    std::vector<DivROMExportOutput> out=e->buildROM(DIV_ROM_AMIGA_VALIDATION);
    if (workingDirROMExport.size()>0) {
      if (workingDirROMExport[workingDirROMExport.size()-1]!=DIR_SEPARATOR) workingDirROMExport+=DIR_SEPARATOR_STR;
    }
    for (DivROMExportOutput& i: out) {
      String path=workingDirROMExport+i.name;
      FILE* outFile=ps_fopen(path.c_str(),"wb");
      if (outFile!=NULL) {
        fwrite(i.data->getFinalBuf(),1,i.data->size(),outFile);
        fclose(outFile);
      }
      i.data->finish();
      delete i.data;
    }
    showError(fmt::sprintf("Done! Baked %d files.",(int)out.size()));
    ImGui::CloseCurrentPopup();
  }
}

void FurnaceGUI::drawExportText(bool onWindow) {
  exitDisabledTimer=1;

  ImGui::Text(
    "this option exports the song to a text file.\n"
  );
  if (onWindow) {
    ImGui::Separator();
    if (ImGui::Button("Cancel",ImVec2(200.0f*dpiScale,0))) ImGui::CloseCurrentPopup();
    ImGui::SameLine();
  }
  if (ImGui::Button("Export",ImVec2(200.0f*dpiScale,0))) {
    openFileDialog(GUI_FILE_EXPORT_TEXT);
    ImGui::CloseCurrentPopup();
  }
}

void FurnaceGUI::drawExportCommand(bool onWindow) {
  exitDisabledTimer=1;
  
  ImGui::Text(
    "this option exports a text or binary file which\n"
    "contains a dump of the internal command stream\n"
    "produced when playing the song.\n\n"

    "technical/development use only!"
  );
  if (onWindow) {
    ImGui::Separator();
    if (ImGui::Button("Cancel",ImVec2(133.3f*dpiScale,0))) ImGui::CloseCurrentPopup();
    ImGui::SameLine();
  }
  if (ImGui::Button("Export (binary)",ImVec2(133.3f*dpiScale,0))) {
    openFileDialog(GUI_FILE_EXPORT_CMDSTREAM_BINARY);
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("Export (text)",ImVec2(133.3f*dpiScale,0))) {
    openFileDialog(GUI_FILE_EXPORT_CMDSTREAM);
    ImGui::CloseCurrentPopup();
  }
}

void FurnaceGUI::drawExport() {
  if (settings.exportOptionsLayout==1 || curExportType==GUI_EXPORT_NONE) {
    if (ImGui::BeginTabBar("ExportTypes")) {
      if (ImGui::BeginTabItem("Audio")) {
        drawExportAudio(true);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("VGM")) {
        drawExportVGM(true);
        ImGui::EndTabItem();
      }
      int numZSMCompat=0;
      for (int i=0; i<e->song.systemLen; i++) {
        if ((e->song.system[i]==DIV_SYSTEM_VERA) || (e->song.system[i]==DIV_SYSTEM_YM2151)) numZSMCompat++;
      }
      if (numZSMCompat>0) {
        if (ImGui::BeginTabItem("ZSM")) {
          drawExportZSM(true);
          ImGui::EndTabItem();
        }
      }
      int numAmiga=0;
      for (int i=0; i<e->song.systemLen; i++) {
        if (e->song.system[i]==DIV_SYSTEM_AMIGA) numAmiga++;
      }
      if (numAmiga && settings.iCannotWait) {
        if (ImGui::BeginTabItem("Amiga Validation")) {
          drawExportAmigaVal(true);
          ImGui::EndTabItem();
        }
      }
      if (ImGui::BeginTabItem("Text")) {
        drawExportText(true);
        ImGui::EndTabItem();
      }
      if (ImGui::BeginTabItem("Command Stream")) {
        drawExportCommand(true);
        ImGui::EndTabItem();
      }
      ImGui::EndTabBar();
    }
  } else switch (curExportType) {
    case GUI_EXPORT_AUDIO:
      drawExportAudio(true);
      break;
    case GUI_EXPORT_VGM:
      drawExportVGM(true);
      break;
    case GUI_EXPORT_ZSM:
      drawExportZSM(true);
      break;
    case GUI_EXPORT_AMIGA_VAL:
      drawExportAmigaVal(true);
      break;
    case GUI_EXPORT_TEXT:
      drawExportText(true);
      break;
    case GUI_EXPORT_CMD_STREAM:
      drawExportCommand(true);
      break;
    default:
      ImGui::Text("congratulations! you've unlocked a secret panel.");
      if (ImGui::Button("Toggle hidden systems")) {
        settings.hiddenSystems=!settings.hiddenSystems;
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::Button("Toggle all instrument types")) {
        settings.displayAllInsTypes=!settings.displayAllInsTypes;
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::Button("Set pitch linearity to Partial")) {
        e->song.linearPitch=1;
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::Button("Enable multi-threading settings")) {
        settings.showPool=1;
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::Button("Set fat to max")) {
        ImGuiStyle& sty=ImGui::GetStyle();
        sty.FramePadding=ImVec2(20.0f*dpiScale,20.0f*dpiScale);
        sty.ItemSpacing=ImVec2(10.0f*dpiScale,10.0f*dpiScale);
        sty.ItemInnerSpacing=ImVec2(10.0f*dpiScale,10.0f*dpiScale);
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::Button("Set muscle and fat to zero")) {
        ImGuiStyle& sty=ImGui::GetStyle();
        sty.FramePadding=ImVec2(0,0);
        sty.ItemSpacing=ImVec2(0,0);
        sty.ItemInnerSpacing=ImVec2(0,0);
        ImGui::CloseCurrentPopup();
      }
      if (ImGui::Button("Tell tildearrow this must be a mistake")) {
        showError("yeah, it's a bug. write a bug report in the GitHub page and tell me how did you get here.");
        ImGui::CloseCurrentPopup();
      }
      break;
  }
}
