<script setup lang="ts">
import { TooltipProvider } from '@/components/ui/tooltip'
import { ref, onMounted, computed, h } from 'vue'
import { DataTable, type ColumnDef } from '@/components/ui/data-table'
import DataTableHeader from '@/components/ui/data-table/DataTableHeader.vue'
import { Checkbox } from '@/components/ui/checkbox'
import { Button } from '@/components/ui/button'
import { Badge } from '@/components/ui/badge'
import { Tooltip, TooltipTrigger, TooltipContent } from '@/components/ui/tooltip'
import { useProductNotificationStore, type ProductNotification } from '@/stores/productnotification'

// Utility to format ISO timestamps
function formatDateTime(iso: string) {
  return new Date(iso).toLocaleString()
}

// Pinia store for notifications
const notificationStore = useProductNotificationStore()

// Fetch alerts on mount
onMounted(() => {
  notificationStore.fetchAlerts()
})

// Table data and state
const data = computed<ProductNotification[]>(() => notificationStore.alerts)
const loading = computed(() => notificationStore.alertsLoading)
const error = computed(() => notificationStore.alertsError)

// Selected IDs for acknowledgement
const selectedIds = ref<number[]>([])
const hasSelection = computed(() => selectedIds.value.length > 0)

async function acknowledgeSelected() {
  for (const id of selectedIds.value) {
    try {
      await notificationStore.acknowledgeNotification(id)
    } catch (e) {
      console.error(`Failed to acknowledge ${id}:`, e)
    }
  }
  selectedIds.value = []
  notificationStore.fetchAlerts()
}

// Table columns with selection checkbox, formatted date, and message with tooltip
const columns: ColumnDef<ProductNotification>[] = [
  {
    id: 'select',
    header: ({ table }) =>
      h(Checkbox, {
        checked: selectedIds.value.length === data.value.length && data.value.length > 0,
        indeterminate:
          selectedIds.value.length > 0 && selectedIds.value.length < data.value.length,
        'onUpdate:checked': (val: boolean) => {
          selectedIds.value = val ? data.value.map((a) => a.id) : []
        },
        ariaLabel: 'Select All',
      }),
    cell: ({ row }) => {
      const id = row.original.id
      return h(Checkbox, {
        checked: selectedIds.value.includes(id),
        'onUpdate:checked': (val: boolean) => {
          if (val) selectedIds.value.push(id)
          else selectedIds.value = selectedIds.value.filter((i) => i !== id)
        },
        ariaLabel: `Select alert ${id}`,
        enableSorting: false,
        enableHiding: false,
      })
    },
  },
  { accessorKey: 'id', header: ({ column }) => h(DataTableHeader, { column, title: 'Alert ID' }) },
  { accessorKey: 'productevent', header: ({ column }) => h(DataTableHeader, { column, title: 'Event ID' }) },
  { accessorKey: 'order', header: ({ column }) => h(DataTableHeader, { column, title: 'Order #' }) },
  {
    accessorKey: 'requirement',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Requirement' }),
    cell: ({ getValue }) => h(Badge, { variant: 'warning' }, () => String(getValue())),
  },
  {
    accessorKey: 'timestamp',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Timestamp' }),
    cell: ({ row }) => formatDateTime(row.original.timestamp),
  },
  {
    accessorKey: 'message',
    header: ({ column }) => h(DataTableHeader, { column, title: 'Message' }),
    cell: ({ getValue }) => {
      const msg = String(getValue())
      // Truncate with tooltip for long messages
      return h(
        Tooltip,
        {},
        {
          default: () => h(
            TooltipTrigger,
            { asChild: true },
            () => h('div', { class: 'truncate max-w-xs' }, msg)
          ),
          content: () => h(TooltipContent, null, msg),
        }
      )
    },
  },
]
</script>

<template>
  <TooltipProvider>
    <div class="space-y-4">
      <!-- Acknowledge button -->
      <div class="flex justify-between items-center mb-4">
        <h2 class="text-lg font-medium">Alerts</h2>
        <Button variant="outline" :disabled="!hasSelection" @click="acknowledgeSelected">
          Acknowledge Selected
        </Button>
      </div>

      <!-- Table -->
      <div v-if="loading" class="text-center">Loading alerts…</div>
      <div v-else-if="error" class="text-red-500">Error: {{ error }}</div>
      <div v-else>
        <DataTable :columns="columns" :data="data" />
      </div>
    </div>
  </TooltipProvider>
</template>
