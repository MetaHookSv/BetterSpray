import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest


spec = importlib.util.spec_from_file_location(
    'gamedata_sync_tags', Path(__file__).parents[1] / 'sync-gamedata.py')
sync = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = sync
spec.loader.exec_module(sync)


class GameVersionTagTests(unittest.TestCase):
    def parse_index(self, game_version, content_addressed=False):
        sha256 = '0' * 64
        file_name = f'{game_version}.{sha256}.json' if content_addressed else f'{game_version}.json'
        document = {'schemaVersion': 4, 'versions': [{
            'gameVersion': game_version, 'url': file_name, 'sha256': sha256,
            'size': 1, 'snapshotSchemaVersion': 8, 'fileCount': 1,
            'lastPublishTime': '2026-10-24T00:00:00Z',
        }]}
        return sync.parse_index(json.dumps(document).encode('utf-8'), 'fixture index')

    def load_manifest(self, game_version, field='gameVersions'):
        document = {
            'schemaVersion': 1, 'name': 'fixture', 'gameVersions': ['hl-8684'],
            'symbols': {'engine': {'baseline': 'global'}},
            'symbolExemptions': {'baseline': []},
            'conditionalGroups': [{
                'when': [], 'exempt': [], 'symbols': {'client': {'mouse': 'global'}},
            }],
        }
        if field == 'gameVersions':
            document[field].append(game_version)
        elif field == 'symbolExemptions':
            document[field]['baseline'].append(game_version)
        else:
            document['conditionalGroups'][0][field].append(game_version)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'manifest.json'
            path.write_text(json.dumps(document), encoding='utf-8')
            return sync.load_manifest(path)

    def test_index_accepts_decay_and_existing_versioned_tags(self):
        for game_version in ('decay', 'hl-8684', 'hl-10210', 'cstrikegameui-8684', 'hl-8684a'):
            for content_addressed in (False, True):
                with self.subTest(game_version=game_version, content_addressed=content_addressed):
                    index = self.parse_index(game_version, content_addressed)
                    self.assertEqual([game_version], [entry.game_version for entry in index.entries])

    def test_index_rejects_unknown_unversioned_and_malformed_tags(self):
        for game_version in ('othermod', 'Decay', 'decay-extra', 'hl-', '../decay', 'decay\n'):
            with self.subTest(game_version=game_version):
                with self.assertRaisesRegex(sync.UpdateError, 'invalid gameVersion'):
                    self.parse_index(game_version)

    def test_manifest_accepts_decay_in_each_version_condition(self):
        for field in ('gameVersions', 'symbolExemptions', 'when', 'exempt'):
            with self.subTest(field=field):
                manifest = self.load_manifest('decay', field)
                if field == 'gameVersions':
                    self.assertEqual(('hl-8684', 'decay'), manifest.game_versions)
                elif field == 'symbolExemptions':
                    self.assertEqual(('decay',), manifest.symbol_exemptions['baseline'])
                else:
                    self.assertEqual(('decay',), getattr(manifest.conditional_groups[0], field))

    def test_manifest_rejects_unknown_unversioned_tags_in_each_version_condition(self):
        for field in ('gameVersions', 'symbolExemptions', 'when', 'exempt'):
            with self.subTest(field=field):
                with self.assertRaises(sync.UpdateError):
                    self.load_manifest('othermod', field)


if __name__ == '__main__':
    unittest.main()
