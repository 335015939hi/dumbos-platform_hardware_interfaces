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
	"strings"

	"github.com/google/blueprint"

	"android/soong/android"
	"android/soong/kernel"
)

type dependencyTag struct {
	blueprint.BaseDependencyTag
	name string
}

var (
	pctx = android.NewPackageContext("android/vintf")

	assembleVintfRule = pctx.AndroidStaticRule("assemble_vintf", blueprint.RuleParams{
		Command:     `${assembleVintfCmd} -i ${inputs} -o ${out}`,
		Description: "assemble_vintf -i ${inputs}",
	}, "inputs")

	kernelConfigTag = dependencyTag{name: "kernel-config"}
)

const (
	relpath = "vintf"
)

type vintfCompatibilityMatrixProperties struct {
	Stem           string
	Srcs           []string
	Kernel_configs []string
}

type vintfCompatibilityMatrixRule struct {
	android.ModuleBase
	properties vintfCompatibilityMatrixProperties

	genFile android.WritablePath
}

func init() {
	pctx.HostBinToolVariable("assembleVintfCmd", "assemble_vintf")
	android.RegisterModuleType("vintf_compatibility_matrix", vintfCompatibilityMatrixFactory)
}

func vintfCompatibilityMatrixFactory() android.Module {
	g := &vintfCompatibilityMatrixRule{}
	g.AddProperties(&g.properties)
	android.InitAndroidArchModule(g, android.DeviceSupported, android.MultilibCommon)
	return g
}

var _ android.AndroidMkDataProvider = (*vintfCompatibilityMatrixRule)(nil)

func (g *vintfCompatibilityMatrixRule) DepsMutator(ctx android.BottomUpMutatorContext) {
	android.ExtractSourcesDeps(ctx, g.properties.Srcs)
	ctx.AddDependency(ctx.Module(), kernelConfigTag, g.properties.Kernel_configs...)
}

func (g *vintfCompatibilityMatrixRule) GenerateAndroidBuildActions(ctx android.ModuleContext) {

	outputFilename := g.properties.Stem
	if len(outputFilename) == 0 {
		outputFilename = g.Name()
	}

	var inputPaths android.Paths = make([]android.Path, len(g.properties.Srcs)+len(g.properties.Kernel_configs))
	for i, src := range g.properties.Srcs {
		inputPaths[i] = android.PathForModuleSrc(ctx, src)
	}
	i := len(g.properties.Srcs)
	ctx.VisitDirectDepsWithTag(kernelConfigTag, func(m android.Module) {
		if k, ok := m.(*kernel.KernelConfigRule); ok {
			inputPaths[i] = k.OutputPath()
		} else {
			ctx.PropertyErrorf("kernel_config",
				"module %q is not a kernel_config", ctx.OtherModuleName(m))
		}
		i++
	})

	g.genFile = android.PathForModuleGen(ctx, outputFilename)

	ctx.Build(pctx, android.BuildParams{
		Rule:        assembleVintfRule,
		Description: "Framework Compatibility Matrix",
		Implicits:   inputPaths,
		Output:      g.genFile,
		Args: map[string]string{
			"inputs": strings.Join(inputPaths.Strings(), ":"),
		},
	})

	ctx.InstallFile(android.PathForModuleInstall(ctx, "etc", relpath), outputFilename, g.genFile)
}

func (g *vintfCompatibilityMatrixRule) AndroidMk() android.AndroidMkData {
	return android.AndroidMkData{
		Class:      "ETC",
		OutputFile: android.OptionalPathForPath(g.genFile),
		Extra: []android.AndroidMkExtraFunc{
			func(w io.Writer, outputFile android.Path) {
				fmt.Fprintln(w, "LOCAL_MODULE_RELATIVE_PATH :=", relpath)
				if len(g.properties.Stem) > 0 {
					fmt.Fprintln(w, "LOCAL_MODULE_STEM :=", g.properties.Stem)
				}
			},
		},
	}
}
