#!/usr/bin/env python3
"""Dialog event answer scopes after the reproduced receiver family plateau."""
import argparse
import itertools
import json
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('parent_checkpoint', type=Path)
parser.add_argument('output', type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'src/kb.cpp').read_text()

def function(text):
    start = text.index('int eventWindowHandler(message& msg)\n{')
    return text[start:text.index('\n}\n', start) + 2]
body = function(source)
checkpoint = json.loads(args.parent_checkpoint.read_text())
parents = []
for row in checkpoint['elites']:
    path = args.parent_checkpoint.parent / 'candidates' / row['id'] / 'repeat/tree/src/kb.cpp'
    if path.is_file() and function(path.read_text()) == body:
        parents.append(row['id'])
if not parents:
    raise ValueError('event body does not equal a reproduced parent')

answer = '''        case DIALOG_RETURN_OK:
            if (g_normalDialogMbType == NORMAL_DIALOG_CHOOSE_OPTIONAL
                || g_normalDialogMbType == NORMAL_DIALOG_CHOOSE) {
                g_windowManager->m_dialogReturn = g_normalDialogSelection;
                msg.m_codeY = 10;
                msg.m_codeX = 10;
                g_dialogDeadline697784 = 0;
                return MESSAGE_DISPATCH_FORWARD;
            }
            // fall through'''
explicit = '''        case DIALOG_RETURN_OK: {
            if (g_normalDialogMbType == NORMAL_DIALOG_CHOOSE_OPTIONAL
                || g_normalDialogMbType == NORMAL_DIALOG_CHOOSE) {
                g_windowManager->m_dialogReturn = g_normalDialogSelection;
            } else {
                g_windowManager->m_dialogReturn = msg.m_codeY;
            }
            msg.m_codeY = 10;
            msg.m_codeX = 10;
            g_dialogDeadline697784 = 0;
            return MESSAGE_DISPATCH_FORWARD;
        }'''
assert body.count(answer) == 1
options = []
for answer_scope, guard, selection in itertools.product(range(2), range(2), range(3)):
    candidate = body.replace(answer, explicit) if answer_scope else body
    start = candidate.index('        case DIALOG_RETURN_OK:')
    end = candidate.index('        case g_dialogReturnClose:', start)
    arm = candidate[start:end]
    if selection:
        condition = '''g_normalDialogMbType == NORMAL_DIALOG_CHOOSE_OPTIONAL
                || g_normalDialogMbType == NORMAL_DIALOG_CHOOSE'''
        if selection == 1:
            arm = arm.replace(condition, 'dialogType == NORMAL_DIALOG_CHOOSE_OPTIONAL\n                || dialogType == NORMAL_DIALOG_CHOOSE')
            declaration = '            const int dialogType = g_normalDialogMbType;\n'
        else:
            arm = arm.replace(condition, 'choosesOption')
            declaration = '            bool choosesOption = ' + condition + ';\n'
        if answer_scope:
            arm = arm.replace('        case DIALOG_RETURN_OK: {\n', '        case DIALOG_RETURN_OK: {\n' + declaration)
        else:
            arm = arm.replace('        case DIALOG_RETURN_OK:\n', '        case DIALOG_RETURN_OK: {\n' + declaration)
            arm = arm.replace('            // fall through', '        }\n            // fall through')
    candidate = candidate[:start] + arm + candidate[end:]
    if guard:
        anchor = '''    if (msg.m_id == MESSAGE_WIDGET
        && msg.m_codeX == widget::WIDGET_DESELECT) {'''
        candidate = candidate.replace(anchor, '''    if (msg.m_id == MESSAGE_WIDGET) {
      if (msg.m_codeX == widget::WIDGET_DESELECT) {''')
        candidate = candidate.replace('    return MESSAGE_DISPATCH_CONSUME;', '    }\n    return MESSAGE_DISPATCH_CONSUME;')
    row = {'name': f'answer-{answer_scope}-guard-{guard}-selection-{selection}'}
    if answer_scope or guard or selection:
        row['replace'] = candidate
    options.append(row)
args.output.write_text(json.dumps({
    'schema': 1, 'source': 'src/kb.cpp', 'units': ['kb'],
    'axes': [{'name': 'answer-path-and-predicate-scopes', 'find': body, 'options': options}],
    'evidence': [
        'Parent checkpoint: ' + str(args.parent_checkpoint),
        'Reproduced parents: ' + ', '.join(parents),
        'DC 2593..2603 gives an OK-arm if/else with distinct dialogReturn assignments and shared message/deadline normalization. Test that source scope against the current fallthrough/duplicated-tail form.',
        'DC nested scopes begin at e20a2 and e20ae for message kind and deselect checks. Test nested predicates without changing their short-circuit order.',
        'Retail exact CFG has 20 blocks and all 15 call decisions agree. Only the selected-answer assignment keeps a different EAX/ECX/EDX binding; test actual condition-result lifetime, not more copies of the assignment RHS.',
        '12 finite states preserve the message reference, all named helper boundaries, answer values and source-order stores. No synthetic operations or inliner pins.',
    ],
}, indent=2) + '\n')
