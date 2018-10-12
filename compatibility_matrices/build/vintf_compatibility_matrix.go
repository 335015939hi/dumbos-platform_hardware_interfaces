// Copyright (C) 2018 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package vintf

import (
	"fmt"
	"io"
	"path/filepath"
	"strings"

	"github.com/google/blueprint"
	"github.com/google/blueprint/pathtools"

	"android/soong/android"
	"android/soong/kernel"
)

type dependencyTag struct {
	blueprint.BaseDependencyTag
	name string
}

var (
	pctx = android.NewPackageContext("android/vintf")

	kernelConfigTag = dependencyTag{name: "kernel-config"}
)

type vintfCompatibilityMatrixProperties struct {
	Stem           string
	Srcs           []string
	Kernel_configs []string
}

type vintfCompatibilityMatrixRule struct {
	android.ModuleBase
	properties vintfCompatibilityMatrixProperties

	inputs []string
	genDir string
}

func init() {
	pctx.HostBinToolVariable("assembleVintfCmd", "assemble_vintf")
	android.RegisterModuleType("vintf_compatibility_matrix", vintfCompatibilityMatrixFactory)
}

func vintfCompatibilityMatrixFactory() android.Module {
	g := &vintfCompatibilityMatrixRule{}
	g.AddProperties(&g.properties)
	android.InitAndroidModule(g)
	return g
}

var _ android.AndroidMkDataProvider = (*vintfCompatibilityMatrixRule)(nil)

func (g *vintfCompatibilityMatrixRule) DepsMutator(ctx android.BottomUpMutatorContext) {
	android.ExtractSourcesDeps(ctx, g.properties.Srcs)
	ctx.AddVariationDependencies([]blueprint.Variation{
		{Mutator: "arch", Variation: ctx.Config().BuildOsVariant},
	}, nil, "assemble_vintf")
	ctx.AddDependency(ctx.Module(), kernelConfigTag, g.properties.Kernel_configs...)
}

func (g *vintfCompatibilityMatrixRule) GenerateAndroidBuildActions(ctx android.ModuleContext) {
	g.inputs = pathtools.PrefixPaths(g.properties.Srcs, android.PathForModuleSrc(ctx).String())

	ctx.VisitDirectDepsWithTag(kernelConfigTag, func(m android.Module) {
		if k, ok := m.(*kernel.KernelConfigRule); ok {
			g.inputs = append(g.inputs, k.OutputCompatibilityMatrixFile())
		} else {
			ctx.PropertyErrorf("kernel_config",
				"module %q is a kernel_config", ctx.OtherModuleName(m))
		}
	})
	g.genDir = android.PathForModuleGen(ctx).String()
}

func (g *vintfCompatibilityMatrixRule) AndroidMk() android.AndroidMkData {
	return android.AndroidMkData{
		Custom: func(w io.Writer, name, prefix, moduleDir string, data android.AndroidMkData) {
			fmt.Fprintln(w)
			fmt.Fprintln(w, "include $(CLEAR_VARS)")
			fmt.Fprintln(w, "LOCAL_PATH :=", moduleDir)
			fmt.Fprintln(w, "LOCAL_MODULE :=", g.Name())
			fmt.Fprintln(w, "LOCAL_MODULE_STEM :=", g.properties.Stem)
			fmt.Fprintln(w, "LOCAL_MODULE_CLASS := ETC")
			fmt.Fprintln(w, "LOCAL_MODULE_PATH := $(TARGET_OUT)/etc/vintf")
			fmt.Fprintln(w, "GEN := ", filepath.Join(g.genDir, g.properties.Stem))
			fmt.Fprint(w, "$(GEN): ")
			fmt.Fprintln(w, "$(HOST_OUT_EXECUTABLES)/assemble_vintf", strings.Join(g.inputs, " "))
			fmt.Fprintln(w, "\t$(HOST_OUT_EXECUTABLES)/assemble_vintf",
				"-i", strings.Join(g.inputs, ":"),
				"-o", "$@")
			fmt.Fprintln(w, "LOCAL_PREBUILT_MODULE_FILE := $(GEN)")
			fmt.Fprintln(w, "GEN :=")
			fmt.Fprintln(w, "include $(BUILD_PREBUILT)")
		},
	}
}
