//! Native variable merging, connected-component order and backjumping.

use super::{random::Random, unique, ClockRead, Fault, Issue, Problem, NATIVE_TERRAIN};

#[derive(Debug)]
struct Variable {
    towns: Vec<usize>,
    terrains: Vec<usize>,
    town_domain: [bool; 12],
    terrain_domain: [bool; 10],
    values: Vec<u8>,
    current: usize,
    // Different town type, different terrain, matching terrain, in native order.
    neighbours: [Vec<usize>; 3],
}
impl Variable {
    fn new() -> Self {
        Self {
            towns: Vec::new(),
            terrains: Vec::new(),
            town_domain: [true; 12],
            terrain_domain: [true; 10],
            values: Vec::new(),
            current: 0,
            neighbours: Default::default(),
        }
    }
    fn degree(&self) -> usize {
        self.neighbours.iter().map(Vec::len).sum()
    }
    fn value(&self) -> u8 {
        self.values.get(self.current).copied().unwrap_or(0)
    }
    fn terrain(&self) -> u8 {
        if self.towns.is_empty() {
            self.value()
        } else {
            NATIVE_TERRAIN[usize::from(self.value())]
        }
    }
    fn filter_terrain(&mut self, terrains: [bool; 10]) {
        for (town, value) in self.town_domain.iter_mut().enumerate() {
            *value &= terrains[usize::from(NATIVE_TERRAIN[town])];
        }
    }
}

pub(super) struct Graph {
    variables: Vec<Variable>,
    town_variables: Vec<Option<usize>>,
    terrain_variables: Vec<Option<usize>>,
}

#[derive(Clone, Copy)]
enum Add {
    Town(usize, bool),
    Terrain(usize),
}

impl Graph {
    // RVA 0x20e070 / 0x20df40: preserve recursive operation order with an owned
    // stack, so template graph depth cannot overflow the Rust call stack.
    fn add(&mut self, problem: &Problem, variable: usize, first: Add) {
        let mut pending = vec![first];
        while let Some(next) = pending.pop() {
            let value = &mut self.variables[variable];
            match next {
                Add::Town(index, only_first) => {
                    if self.town_variables[index].is_some()
                        || (only_first && !value.towns.is_empty())
                    {
                        continue;
                    }
                    if value.towns.is_empty() && !value.terrains.is_empty() {
                        value.filter_terrain(value.terrain_domain);
                    }
                    value.towns.push(index);
                    self.town_variables[index] = Some(variable);
                    let town = &problem.towns[index];
                    for (allowed, &keep) in value.town_domain.iter_mut().zip(&town.domain) {
                        *allowed &= keep;
                    }
                    pending.extend(town.matched.iter().rev().copied().map(Add::Terrain));
                    pending.extend(town.same.iter().rev().map(|&i| Add::Town(i, false)));
                }
                Add::Terrain(index) => {
                    if self.terrain_variables[index].is_some() {
                        continue;
                    }
                    value.terrains.push(index);
                    self.terrain_variables[index] = Some(variable);
                    let terrain = &problem.terrains[index];
                    if value.towns.is_empty() {
                        for (allowed, &keep) in value.terrain_domain.iter_mut().zip(&terrain.domain)
                        {
                            *allowed &= keep;
                        }
                    } else {
                        value.filter_terrain(terrain.domain);
                    }
                    pending.extend(terrain.matched.iter().rev().map(|&i| Add::Town(i, true)));
                    pending.extend(terrain.same.iter().rev().copied().map(Add::Terrain));
                }
            }
        }
    }

    fn link(&mut self, a: usize, b: usize, kind: usize) {
        unique(&mut self.variables[a].neighbours[kind], b);
        unique(&mut self.variables[b].neighbours[kind], a);
    }

    // RVA 0x20cc00. Even contradictions discovered here proceed to search;
    // the enclosing solve checks fatal diagnostics before, not after, this call.
    pub fn build(problem: &mut Problem) -> Self {
        let mut graph = Self {
            variables: Vec::new(),
            town_variables: vec![None; problem.towns.len()],
            terrain_variables: vec![None; problem.terrains.len()],
        };
        for index in 0..problem.towns.len() {
            if graph.town_variables[index].is_none() {
                let variable = graph.variables.len();
                graph.variables.push(Variable::new());
                graph.add(problem, variable, Add::Town(index, false));
            }
        }
        for index in 0..problem.terrains.len() {
            if graph.terrain_variables[index].is_none() {
                let variable = graph.variables.len();
                graph.variables.push(Variable::new());
                graph.add(problem, variable, Add::Terrain(index));
            }
        }
        for (index, town) in problem.towns.iter().enumerate() {
            let a = graph.town_variables[index].unwrap();
            for &other in &town.different {
                let b = graph.town_variables[other].unwrap();
                if a == b {
                    let issue =
                        Issue::IncompatibleTowns(problem.town_id(index), problem.town_id(other));
                    unique(&mut problem.issues, issue);
                } else {
                    graph.link(a, b, 0);
                }
            }
            for &other in &town.unmatched {
                let b = graph.terrain_variables[other].unwrap();
                if a == b {
                    let issue = Issue::IncompatibleTownTerrain(
                        problem.town_id(index),
                        problem.terrain_id(other),
                    );
                    unique(&mut problem.issues, issue);
                } else {
                    graph.link(a, b, 1);
                }
            }
            for &other in &town.matched {
                let b = graph.terrain_variables[other].unwrap();
                if a != b {
                    graph.link(a, b, 2);
                }
            }
        }
        for (index, terrain) in problem.terrains.iter().enumerate() {
            let a = graph.terrain_variables[index].unwrap();
            for &other in &terrain.different {
                let b = graph.terrain_variables[other].unwrap();
                if a == b {
                    let issue = Issue::IncompatibleTerrains(
                        problem.terrain_id(index),
                        problem.terrain_id(other),
                    );
                    unique(&mut problem.issues, issue);
                } else {
                    graph.link(a, b, 1);
                }
            }
            for &other in &terrain.unmatched {
                let b = graph.town_variables[other].unwrap();
                if a == b {
                    let issue = Issue::IncompatibleTownTerrain(
                        problem.town_id(other),
                        problem.terrain_id(index),
                    );
                    unique(&mut problem.issues, issue);
                } else {
                    graph.link(a, b, 1);
                }
            }
            for &other in &terrain.matched {
                let b = graph.town_variables[other].unwrap();
                if a != b {
                    graph.link(b, a, 2);
                }
            }
        }
        graph
    }

    pub fn town_value(&self, town: usize) -> u8 {
        self.variables[self.town_variables[town].unwrap()].value()
    }
    pub fn terrain_value(&self, terrain: usize) -> u8 {
        let variable = &self.variables[self.terrain_variables[terrain].unwrap()];
        // Native fallback for empty values is zero, even on a town variable.
        if variable.values.is_empty() {
            0
        } else {
            variable.terrain()
        }
    }

    fn conflicts(&self, id: usize) -> bool {
        let value = &self.variables[id];
        value.neighbours[0].iter().any(|&other| {
            let other = &self.variables[other];
            value.value()
                == if value.towns.is_empty() {
                    other.terrain()
                } else {
                    other.value()
                }
        }) || value.neighbours[1]
            .iter()
            .any(|&other| value.terrain() == self.variables[other].terrain())
            || value.neighbours[2]
                .iter()
                .any(|&other| value.terrain() != self.variables[other].terrain())
    }

    // RVAs 0x20ef30 / 0x20e1d0. Components start at the lowest remaining ID;
    // depth-first collection visits the three neighbour lists in source order.
    pub fn search(
        &mut self,
        random: &mut Random,
        issues: &mut Vec<Issue>,
        clock: &mut impl FnMut(ClockRead) -> Option<i64>,
    ) -> Result<(), Fault> {
        let mut visited = vec![false; self.variables.len()];
        let mut ordinal = 0;
        for first in 0..self.variables.len() {
            if visited[first] {
                continue;
            }
            let mut component = Vec::new();
            let mut pending = vec![first];
            while let Some(index) = pending.pop() {
                if visited[index] {
                    continue;
                }
                visited[index] = true;
                component.push(index);
                for list in self.variables[index].neighbours.iter().rev() {
                    pending.extend(list.iter().rev().copied());
                }
            }
            if let Some(issue) = self.search_component(&mut component, ordinal, random, clock)? {
                unique(issues, issue);
            }
            ordinal += 1;
        }
        Ok(())
    }

    // RVA 0x20f1b0. Sorting uses degree descending, then ID descending.
    fn search_component(
        &mut self,
        component: &mut [usize],
        ordinal: usize,
        random: &mut Random,
        clock: &mut impl FnMut(ClockRead) -> Option<i64>,
    ) -> Result<Option<Issue>, Fault> {
        component.sort_unstable_by_key(|&index| {
            std::cmp::Reverse((self.variables[index].degree(), index))
        });
        for (position, &index) in component.iter().enumerate() {
            let variable = &mut self.variables[index];
            for list in &mut variable.neighbours {
                list.retain(|other| !component[position + 1..].contains(other));
            }
            let domain: &[bool] = if variable.towns.is_empty() {
                &variable.terrain_domain
            } else {
                &variable.town_domain
            };
            variable.values = domain
                .iter()
                .enumerate()
                .filter(|&(_, &allowed)| allowed)
                .map(|(i, _)| u8::try_from(i).unwrap())
                .collect();
            random.shuffle(&mut variable.values);
        }
        if component
            .iter()
            .any(|&id| self.variables[id].values.is_empty())
        {
            return Ok(Some(Issue::NoSolution));
        }
        let start_read = ClockRead {
            component: ordinal,
            steps: 0,
        };
        let start = clock(start_read).ok_or(Fault::MissingClock(start_read))?;
        let mut steps = 0_i32;
        let mut position = 0;
        while position < component.len() {
            steps = steps.wrapping_add(1);
            if steps % 500_000 == 0 {
                let read = ClockRead {
                    component: ordinal,
                    steps,
                };
                let now = clock(read).ok_or(Fault::MissingClock(read))?;
                if timed_out(now.wrapping_sub(start)) {
                    return Ok(Some(Issue::Timeout));
                }
            }
            let id = component[position];
            if !self.conflicts(id) {
                position += 1;
                continue;
            }
            if self.variables[id].current + 1 < self.variables[id].values.len() {
                self.variables[id].current += 1;
                continue;
            }
            let mut back = position;
            while back > 0 {
                self.variables[component[back]].current = 0;
                back -= 1;
                let candidate = component[back];
                let earlier = &self.variables[candidate];
                if earlier.current + 1 < earlier.values.len()
                    && self.variables[id]
                        .neighbours
                        .iter()
                        .any(|list| list.contains(&candidate))
                {
                    break;
                }
            }
            let candidate = component[back];
            if position == 0
                || !(self.variables[candidate].current + 1 < self.variables[candidate].values.len()
                    && self.variables[id]
                        .neighbours
                        .iter()
                        .any(|list| list.contains(&candidate)))
            {
                return Ok(Some(Issue::NoSolution));
            }
            self.variables[candidate].current += 1;
            position = back;
        }
        Ok(None)
    }
}

#[expect(
    clippy::cast_precision_loss,
    reason = "native converts the signed FILETIME difference to double"
)]
fn timed_out(elapsed: i64) -> bool {
    elapsed as f64 / 10_000_000.0 > 10.0
}

#[cfg(test)]
mod tests {
    #[test]
    fn native_clock_deadline_is_strict_and_uses_signed_elapsed_ticks() {
        assert!(!super::timed_out(-100));
        assert!(!super::timed_out(100_000_000));
        assert!(super::timed_out(100_000_001));
    }
}
