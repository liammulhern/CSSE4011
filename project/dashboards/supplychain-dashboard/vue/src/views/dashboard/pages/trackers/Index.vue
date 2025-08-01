<script setup lang="ts">
import { h, onMounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { DataTable, type ColumnDef } from '@/components/ui/data-table'
import DataTableHeader from '@/components/ui/data-table/DataTableHeader.vue'
import { Checkbox } from '@/components/ui/checkbox'
import { Button } from '@/components/ui/button'
import { useTrackerStore } from '@/stores/tracker'
import NewMessageForm from '@/views/dashboard/pages/trackers/tracker/NewMessageForm.vue'

// Shape of each tracker row
interface TrackerRow {
  id: number
  key: string
}

const router = useRouter()
const store = useTrackerStore()

// Flatten list of trackers into table rows
type TrackerRowData = TrackerRow
const data = computed<TrackerRowData[]>(() =>
  store.trackers.map(t => ({ id: t.id, key: t.tracker_key }))
)

const loading = computed(() => store.loading)
const error = computed(() => store.error)

onMounted(() => {
  store.fetchTrackers()
})

// Table columns
const columns: ColumnDef<TrackerRowData>[] = [
  {
    accessorKey: 'id',
    header: ({ column }) => h(DataTableHeader, { column, title: '#' }),
  },
  {
    accessorKey: 'key',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Tracker Key' }),
    cell: ({ getValue }) => String(getValue()),
  },
  {
    id: 'actions',
    header: () => 'Actions',
    cell: ({ row }) =>
      h(
        Button,
        {
          variant: 'ghost',
          size: 'sm',
          onClick: () => router.push({ name: 'tracker_index', params: { id: String(row.original.id) } }),
        },
        () => 'View'
      ),
  },
]
</script>

<template>
  <div>
    <page-header title="Trackers">
      <NewMessageForm />
    </page-header>
    <div class="mb-4 flex justify-between items-center">
      <h2 class="text-lg font-medium">Trackers</h2>
      <Button :disabled="loading" @click="store.fetchTrackers()">
        <Icon name="RefreshCw" class="pr-1" />
        Refresh
      </Button>
    </div>

    <div v-if="loading" class="py-4 text-center">
      Loading trackers…
    </div>
    <div v-else-if="error" class="py-4 text-red-500">
      Error: {{ error }}
    </div>
    <div v-else>
      <DataTable :columns="columns" :data="data" />
    </div>
  </div>
</template>
