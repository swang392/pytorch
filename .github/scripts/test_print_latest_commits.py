from unittest import TestCase, main, mock
from typing import Any, List, Dict
from print_latest_commits import isGreen, WorkflowCheck

workflowNames = [
    "pull",
    "trunk",
    "Lint",
    "linux-binary-libtorch-pre-cxx11",
    "android-tests",
    "windows-binary-wheel",
    "periodic",
    "docker-release-builds",
    "nightly",
    "pr-labels",
    "Close stale pull requests",
    "Update S3 HTML indices for download.pytorch.org",
    "Create Release"
]

class TestChecks:
    def make_test_checks(self) -> List[Dict[str, Any]]:
        workflow_checks = []
        for i in range(len(workflowNames)):
            workflow_checks.append(WorkflowCheck(
                workflowName=workflowNames[i],
                name="test/job",
                jobName="job",
                conclusion="success",
            )._asdict())
        return workflow_checks

class TestPrintCommits(TestCase):
    @mock.patch('print_latest_commits.get_commit_results', return_value=TestChecks().make_test_checks())
    def test_all_successful(self, mock_get_commit_results: Any) -> None:
        "Test with workflows are successful"
        workflow_checks = mock_get_commit_results()
        self.assertTrue(isGreen("sha", workflow_checks))

    @mock.patch('print_latest_commits.get_commit_results', return_value=TestChecks().make_test_checks())
    def test_necessary_successful(self, mock_get_commit_results: Any) -> None:
        "Test with necessary workflows are successful"
        workflow_checks = mock_get_commit_results()
        for i in range(8, 13):
            workflow_checks[i]['conclusion'] = 'failed'
        self.assertTrue(isGreen("sha", workflow_checks))

    @mock.patch('print_latest_commits.get_commit_results', return_value=TestChecks().make_test_checks())
    def test_necessary_skipped(self, mock_get_commit_results: Any) -> None:
        "Test with necessary job (ex: pull) skipped"
        workflow_checks = mock_get_commit_results()
        workflow_checks[0]['conclusion'] = 'skipped'
        self.assertEqual(isGreen("sha", workflow_checks), "pull checks were not successful")

    @mock.patch('print_latest_commits.get_commit_results', return_value=TestChecks().make_test_checks())
    def test_skippable_skipped(self, mock_get_commit_results: Any) -> None:
        "Test with skippable jobs (periodic and docker-release-builds skipped"
        workflow_checks = mock_get_commit_results()
        workflow_checks[6]['conclusion'] = 'skipped'
        workflow_checks[7]['conclusion'] = 'skipped'
        self.assertTrue(isGreen("sha", workflow_checks))

    @mock.patch('print_latest_commits.get_commit_results', return_value=TestChecks().make_test_checks())
    def test_necessary_failed(self, mock_get_commit_results: Any) -> None:
        "Test with necessary job (ex: Lint) failed"
        workflow_checks = mock_get_commit_results()
        workflow_checks[2]['conclusion'] = 'failed'
        self.assertEqual(isGreen("sha", workflow_checks), "Lint checks were not successful")

    @mock.patch('print_latest_commits.get_commit_results', return_value=TestChecks().make_test_checks())
    def test_skippable_failed(self, mock_get_commit_results: Any) -> None:
        "Test with skippable job (ex: docker-release-builds) failing"
        workflow_checks = mock_get_commit_results()
        workflow_checks[6]['conclusion'] = 'skipped'
        workflow_checks[7]['conclusion'] = 'failed'
        self.assertEqual(isGreen("sha", workflow_checks), "docker-release-builds checks were not successful")

if __name__ == "__main__":
    main()
