1) *Data dependency granularity*: currently _blocks_ are defined as the quantum of, simultaneously, data allocation/management, dependency relations, and kernel input/output flows. At this point I think it makes sense for all of these notions to be commonly-grounded as such; no level of organization _below_ this seems efficacious. However it will probably be useful in the future to build at least one hierarchical layer on top of this. (_note_: look into StarPU hierarchical partitioning.)
	1) *sub-question*: if data-hierarchy becomes multi-level, will need to rethink naming (i.e. should _blocks_ be the lowest level or a higher-level cluster). No strong feelings at this point.

2) *Data access modes*: the current model is inspired by Rust-style mutable/immutable references as such the Read-Write pair could be more accurately called Read-ReadWrite or alternatively something like Ref-Mut. For the current single node design I do not believe there to be a meaningful difference between a ReadWrite and Write-only mode (_note_: check this more thoroughly) as the read part is essentially free in the shared-memory setting and dependency relations are defined by the write part either way. When moving to a multi-node design a write without reading semantic will become useful.

3) *Program/task/kernel definition within the system*: In current ideation and prototyping these terms are used without a ton of formal-grounding. Roughly speaking:
	      - *Program* refers to a complete set of tasks along with any necessary data allocations, environment setup and teardown, and initial task seeding. It terminates only after all tasks are exhausted (including dynamically spawned ones) i.e. the DAG is fully exhausted.
	1) *sub-question*: quiescence detection

		- *Task* refers to the individual unit of scheduling that makes up the nodes in the program DAG. It is thus defined in a primarily _functional_ way. Exact structural details of what is included in task remain fluid.
		- *Kernel* refers to an individual function (in the abstract sense) that purely performs computation acting on the declared/assigned data blocks.
	as of now tasks are composed of an (environment, kernel) pair (where the kernel is defined within the environment along with the dependency closure) but may be expanded to include a layer handling control flow logic in order to facilitate dynamic task spawning. The degree to which a _program_ is more than just a synecdoche for the root _task_-node of a DAG is also unclear at this point.


