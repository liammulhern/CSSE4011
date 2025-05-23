<script setup lang="ts">
import { ref, onMounted, h } from 'vue'
import http from '@/utils/http'
import { DataTable, type ColumnDef } from '@/components/ui/data-table'
import DataTableHeader from '@/components/ui/data-table/DataTableHeader.vue'
import { Checkbox } from '@/components/ui/checkbox'
import { Badge } from '@/components/ui/badge'
import { Button } from '@/components/ui/button'

// Alert interface matching expected API response
interface ComplianceAlert {
  id: number
  order_id: number
  tracker_id: string | null
  requirement_name: string
  event_type: string
  actual_value: string
  timestamp: string
}

const data = ref<ComplianceAlert[]>([])
const loading = ref(false)
const error = ref<string | null>(null)

/**
 * Fetch compliance alerts from the API
 */
async function fetchAlerts() {
  loading.value = true
  error.value = null
  try {
    const resp = await http.get<{ results: ComplianceAlert[] }>('/api/compliance-alerts/')
    data.value = resp.data.results.map(a => ({
      ...a,
      timestamp: new Date(a.timestamp).toLocaleString(),
    }))
  } catch (e: any) {
    error.value = e.message || String(e)
  } finally {
    loading.value = false
  }
}

onMounted(fetchAlerts)

// Define table columns
const columns: ColumnDef<ComplianceAlert>[] = [
  {
    accessorKey: 'id',
    header: ({ table }) => h(Checkbox, {
      checked: table.getIsAllPageRowsSelected(),
      'onUpdate:checked': val => table.toggleAllPageRowsSelected(!!val),
      ariaLabel: 'Select All',
      class: 'translate-y-0.5',
    }),
    cell: ({ row }) => h(Checkbox, {
      checked: row.getIsSelected(),
      'onUpdate:checked': val => row.toggleSelected(!!val),
      ariaLabel: 'Select row',
      class: 'translate-y-0.5',
      enableSorting: false,
      enableHiding: false,
    }),
  },
  {
    accessorKey: 'id',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Alert ID' }),
  },
  {
    accessorKey: 'order_id',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Order #' }),
  },
  {
    accessorKey: 'tracker_id',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Tracker ID' }),
  },
  {
    accessorKey: 'requirement_name',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Requirement' }),
    cell: ({ getValue }) => h(Badge, { variant: 'warning' }, () => String(getValue())),
  },
  {
    accessorKey: 'event_type',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Event Type' }),
  },
  {
    accessorKey: 'actual_value',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Actual Value' }),
  },
  {
    accessorKey: 'timestamp',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Timestamp' }),
  },
  {
    id: 'actions',
    header: () => ' ',
    cell: () => h(Button, { variant: 'ghost', size: 'sm' }, () => 'Details'),
  }
]
</script>

<template>
  <div>
    <div v-if="loading" class="text-center">Loading alerts…</div>
    <div v-else-if="error" class="text-red-500">Error: {{ error }}</div>
    <div v-else>
      <DataTable :columns="columns" :data="data" />
    </div>
  </div>
</template>
