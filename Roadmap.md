# EnigmaEngine Roadmap

This roadmap tracks project-level milestones. It is intentionally higher level than spec-workflow tasks.

Use this file to decide what comes next. Create a `.spec-workflow/specs/{milestone}/` entry only when a milestone is ready to enter Requirements, Design, Tasks, and Implementation.

## Principles

- Prefer clean target architecture over compatibility with earlier internal mistakes.
- Do not add adapters, aliases, forwarding wrappers, or bridge layers only to preserve wrong internal designs.
- Every milestone must end with observable behavior, tests, or tooling that can be run locally and in CI.
- Use `.reference/@unreal_engine_source_code` for Unreal Engine 5.7 container, module, and automation-test patterns.
- Use [Hello Algo](https://github.com/krahets/hello-algo) as the coverage map for data structures and algorithms, not as code to copy.
- Keep generated project files disposable; source generators and tests are the source of truth.

## Milestone Overview

| Milestone | Name | Status | Purpose |
| --- | --- | --- | --- |
| M1 | Engine Automation Test Framework | Complete | Engine-owned C++ automation test framework with BuildTool runner and CI profile. |
| M2 | Core Containers, Data Structures, and Algorithms | Phase 1 Complete | Build UE-style custom containers and algorithm utilities with coverage mapped from Hello Algo. |
| M3 | Editor Runtime and Hot Reload Reinstancing | Planned | Replace first-version hot reload object recreation with Editor/reflection-aware object reinstancing and scoped runtime registrations. |

## M1 Engine Automation Test Framework

### Goal

Create an Enigma-owned C++ automation test framework inspired by Unreal Engine Automation Tests, so low-level Core work can be tested through a stable engine-side runner instead of only root-level CMake/googletest projects.

### Scope

- Add a Developer-only test framework module, recommended path: `Engine/Source/Developer/AutomationTest/`.
- Provide a lightweight base test type, assertion helpers, failure reporting, and test registry.
- Provide UE-style registration macros, such as an Enigma equivalent of `IMPLEMENT_SIMPLE_AUTOMATION_TEST`.
- Use hierarchical test names, for example `System.Core.Containers.Array`.
- Support filters by module, name prefix, and tags such as `Smoke`, `Unit`, `Integration`, `Slow`, `Perf`.
- Add a CLI or BuildTool entry to list and run automation tests.
- Support CI-friendly console output and a machine-readable report format.
- Define module-local test placement under `Private/Tests/`.
- Keep all automation-test code out of Shipping builds.

### Non-Goals

- Do not maintain a long-term compatibility layer for the current root `Tests/` layout.
- Do not replace every existing test in the first implementation.
- Do not introduce Editor-only assumptions before an Editor exists.

### Deliverables

- `AutomationTest` Developer module.
- Automation test runner command.
- Test registration and discovery.
- Example tests for Core and one Runtime/Developer module.
- Documentation for naming, tags, runner usage, and CI invocation.

### Completion Criteria

- A developer can list tests and run a filtered subset from the command line.
- CI can run automation tests without launching a game project.
- At least one Core test is module-local under `Private/Tests/`.
- Obsolete glue introduced during the milestone is removed before completion.

### Spec Candidate

`engine-automation-test-framework`

## M2 Core Containers, Data Structures, and Algorithms

### Goal

Build Enigma-owned Core containers, data structures, and algorithms that follow Unreal Engine-style APIs where appropriate while covering the data-structure and algorithm map described by Hello Algo.

### Scope

Data structure coverage target:

- Sequence storage: `TArray`, static array/span/view equivalents, string or string-view support where needed.
- Linked structures: singly/doubly linked list only if an engine use case proves value.
- Stack and queue: `TStack`, `TQueue`, deque/ring-buffer variants where justified.
- Hashing: `TSet`, `TMap`, hash functions, load factor policy, collision handling, key traits.
- Trees: binary tree utilities for tests/algorithms; production trees only when required by an engine subsystem.
- Heap and priority queue: binary heap utilities and `TPriorityQueue`.
- Graph utilities: adjacency representation, traversal helpers, dependency graph use cases.
- Matrix/grid helpers only when they support engine math, renderer, or algorithm tests.

Algorithm coverage target:

- Complexity utilities and benchmark baselines for containers.
- Search: linear search, binary search, graph BFS/DFS.
- Sorting: insertion/selection/bubble for education/tests; quick/merge/heap sort as practical baselines; defer production sort replacement until profiled.
- Divide and conquer.
- Backtracking.
- Dynamic programming.
- Greedy algorithms.
- Graph algorithms needed by engine systems, such as topological sort and cycle detection.

### Unreal Reference Alignment

- Prefer UE-style names and API shape when it improves engine consistency: `TArray`, `TMap`, `TSet`, `TQueue`, `TBitArray`, `TSparseArray`, array views and allocator-aware containers.
- Do not clone UE internals blindly. Each container must have a clear Enigma use case, tests, and performance target.
- Allocator, iterator, move/copy, invalidation, and ownership semantics must be documented before broad adoption.

### Non-Goals

- Do not replace standard library containers everywhere before benchmarks justify it.
- Do not implement advanced or niche structures without an engine use case.
- Do not keep std-container and Enigma-container wrapper layers only for compatibility.

### Deliverables

- Container headers under `Engine/Source/Runtime/Core/Public/Containers/`.
- Implementations under `Engine/Source/Runtime/Core/Private/Containers/` when not header-only.
- Automation tests under `Core/Private/Tests/` once M1 exists.
- Benchmarks or perf tests for core containers.
- Documentation for API naming, iterator invalidation, allocator behavior, and complexity.

### Completion Criteria

- Core container tests cover construction, destruction, copy/move, allocator behavior, boundary cases, iterator validity, and assertions.
- Algorithms have tests mapped to Hello Algo categories.
- Engine code adopts Enigma containers only in scoped refactors that remove obsolete compatibility wrappers.
- Container replacement does not regress existing module tests.

### Phase 1 Delivered

- `FDefaultAllocator` in `Engine/Source/Runtime/Core/Public/Containers/ContainerAllocationPolicies.h`.
- `TArray` in `Engine/Source/Runtime/Core/Public/Containers/Array.h`.
- `TArrayView` and `TConstArrayView` in `Engine/Source/Runtime/Core/Public/Containers/ArrayView.h`.
- Opt-in `Enigma::Algo::Find` and `Enigma::Algo::Sort` headers under `Engine/Source/Runtime/Core/Public/Algo/`.
- AutomationTest death assertion support through `ENIGMA_EXPECT_FATAL_ASSERT` and `ENIGMA_ASSERT_FATAL_ASSERT`.
- Module-local Core tests under `Engine/Source/Runtime/Core/Private/Tests/Containers/` and `Engine/Source/Runtime/Core/Private/Tests/Algo/`.
- `CoreMinimal.h` includes `Array.h` and `ArrayView.h`; Algo headers remain explicit opt-in includes.

### Phase 1 Validation

Focused validation commands:

```bash
BuildTool automation-test F:/github/EnigmaEngine --engine --run --name-prefix System.AutomationTest.DeathTest --profile local-fast
BuildTool automation-test F:/github/EnigmaEngine --engine --run --name-prefix System.Core.Containers --profile local-fast
BuildTool automation-test F:/github/EnigmaEngine --engine --run --name-prefix System.Core.Algo --profile local-fast
BuildTool automation-test F:/github/EnigmaEngine --engine --run --profile ci-standard
```

### M2 Follow-up Work

- Iterator model: replace phase 1 raw pointer-compatible iterators with a deliberate cross-container iterator policy when `TMap`, `TSet`, `TSparseArray`, or `TBitArray` need shared semantics. Evaluate debug mutation detection, checked iterator diagnostics, iterator category traits, and invalidation diagnostics.
- Allocator roadmap: add inline allocator, fixed allocator, allocator-aware tests, slack/growth policy tuning, allocator stress tests, memory statistics hooks, and profiling integration only after phase 1 usage exposes concrete requirements.
- Container roadmap: implement later containers through separate specs, starting from proven engine use cases. Candidates remain `TMap`, `TSet`, `TQueue`, `TBitArray`, `TSparseArray`, string/name-adjacent containers, heap/priority queue, and graph/dependency containers.
- Algorithm roadmap: add binary search, stable sort, heap utilities, partitioning, projections, topological sort, graph traversal, and remaining Hello Algo categories through scoped specs. Do not present educational algorithms as production replacements until their purpose is explicit.
- Performance roadmap: add repeatable baselines against `std::vector` and standard algorithms before adopting `TArray` or Enigma algorithms in hot engine paths.
- AutomationTest death-test limitation: fatal assertion tests require non-Shipping builds with `DO_CHECK` and the GoogleTest backend. Threadsafe death tests restart the runner as a child process, so the runner must continue preserving GoogleTest internal death-test arguments while rejecting user-facing `--gtest_filter`.
- Adoption roadmap: broad `std::vector` replacement is deferred. Any adoption must be a separate low-risk pilot spec/refactor that updates public APIs, call sites, tests, docs, and benchmarks together.

### Spec Candidate

`core-containers-data-structures-algorithms`

## M3 Editor Runtime and Hot Reload Reinstancing

### Goal

Build the Editor/runtime infrastructure needed to replace the current first-version hot reload workaround with an Unreal-style reload flow: reload-aware module filenames, class/object reinstancing, scoped runtime registrations, and lifecycle-driven tick cleanup.

The current hot reload snapshot DLL and manifest flow is intentionally kept, but object recovery is still temporary because Enigma does not yet have an Editor, UObject-equivalent reflection, class default objects, or object reinstancing.

### Current Temporary Code and Constraints

- `HotReload` currently depends on `Engine` so it can recreate `FGameInstance` after a game module reload. This is marked `[TEST]` in code and should be removed once Editor/runtime object reconstruction exists.
- `FGameEngine::PrepareGameInstanceForHotReload()` destroys the active `FGameInstance` before unloading the old game DLL, then flushes `FTickTaskManager` pending changes. This prevents stale tick function pointers, but it is not a real object reinstancing system.
- `FGameEngine::RecreateGameInstance()` creates a fresh `FGameInstance` from the reloaded module factory and re-runs input setup. Runtime game state is not preserved.
- `FGameEngine::ClearGameInstanceFactory()` is needed because the registered factory can hold callable code from the old DLL. A reload-aware registration system should make this cleanup part of module or reflection lifetime.
- EnigmaArcade manually stores input binding handles and unregisters only its own bindings/contexts during shutdown. This avoids global input subsystem cleanup, but scoped owner-based input registration should replace ad hoc game-side bookkeeping.
- Tick safety currently relies on component destruction unregistering tick functions and an explicit `FTickTaskManager::FlushPendingChanges()` call during hot reload. Hot reload must not tick unrelated subsystems such as input just to clear stale state.
- There is no preservation of live objects, scenes, components, delegates, input actions, or asset references across reload beyond what the current full `GameInstance` rebuild happens to recreate.

### Target Direction

- Introduce an Editor-facing reload coordinator that owns hot reload and live reload policy instead of letting the `HotReload` Developer module directly drive game object reconstruction.
- Add reflection or type metadata sufficient to identify reloadable game classes, map old classes to new classes, and recreate or patch live instances.
- Add object reinstancing for `FGameInstance`, scenes, game objects, components, and future reflected assets where feasible.
- Move runtime registrations to scoped handles with owner/lifetime tracking: delegates, tick functions, input bindings, mapping contexts, and module-level factories should be removable by owner without global subsystem resets.
- Keep tick cleanup lifecycle-driven: component/object unregister paths remove tick functions, and tick manager frame-boundary APIs only apply pending changes without executing gameplay code.
- Preserve the UE-aligned snapshot DLL behavior: hot reload loads versioned DLLs and records the active module filename; normal full builds refresh canonical DLLs and clean old snapshots.

### Non-Goals

- Do not implement legacy UE Hot Reload internals wholesale.
- Do not add compatibility adapters for the current temporary `GameInstance` recreation API.
- Do not make `GameInstance` own subsystem lifecycle. Game code may unregister handles it owns, but subsystem lifetime remains engine-owned.
- Do not attempt state-preserving reinstancing before there is enough reflection/type metadata to make it deterministic and testable.

### Deliverables

- Editor or reload coordinator module with explicit ownership of reload phases.
- Reflection/type metadata for reloadable game-facing classes.
- Reinstancing path for `FGameInstance` and component-based scene objects.
- Scoped registration primitives for delegates, tick functions, input bindings, mapping contexts, and module factories.
- Hot reload diagnostics that report old DLL, new DLL, module filename, recreated or reinstanced objects, and registrations removed.
- Automation tests covering reload phase ordering, scoped registration cleanup, tick pending-flush behavior, and failure rollback.
- Removal of `[TEST]` hot reload hooks from `HotReload`, `HotReload.Build.cs`, and `FGameEngine` once the replacement exists.

### Completion Criteria

- Hot reloading a game module no longer requires `HotReload` to depend on `Engine` for direct `FGameInstance` recreation.
- Reload cleanup does not tick arbitrary subsystems and does not require global input/delegate/tick resets.
- At least `FGameInstance` and component tick registrations can survive or be deterministically reconstructed through the reload coordinator.
- Failed reload leaves the engine in a well-defined state with diagnostics and no dangling old-DLL callbacks.
- Existing HotReload, TickSystem, EnhancedInput, and example project tests pass, with new tests covering the temporary-code removal.

### Spec Candidate

`editor-runtime-hot-reload-reinstancing`

## Backlog

- CI or BuildTool command to validate formatting with repository-level `.clang-format` and `.editorconfig`.
- C++ naming cleanup by module, starting with Core and EnigmaArcade.
- Steering docs tracking in Git if `.spec-workflow/steering/*.md` is intended to be shared.
- Optional BuildTool command to validate roadmap/spec consistency.

## Spec Workflow Usage

Do not create all specs up front.

When a milestone is ready:

1. Pick exactly one milestone.
2. Create one kebab-case spec under `.spec-workflow/specs/`.
3. Follow Requirements -> Design -> Tasks -> Implementation.
4. Use `spec-status` to track progress.
5. Record implementation details through implementation logs.
6. Return to this roadmap only when the milestone is complete or needs scope adjustment.

## References

- Hello Algo: https://github.com/krahets/hello-algo
- Unreal Engine 5.7 local source: `.reference/@unreal_engine_source_code`
- Previous roadmap style reference: `F:/github/UnrealVoxelSolution/Roadmap.md`
