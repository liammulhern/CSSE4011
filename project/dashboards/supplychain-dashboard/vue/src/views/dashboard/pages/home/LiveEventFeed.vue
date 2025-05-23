<script setup lang="ts">
import { ref, onMounted, onBeforeUnmount } from 'vue'
import http from '@/utils/http'
import { Badge } from '@/components/ui/badge'

interface LiveEvent {
  message_id: string
  event_type: string
  timestamp: string
  source: 'product' | 'tracker'
  ref_id: number | string | null
}

const events = ref<LiveEvent[]>([])
const loading = ref(false)
const error = ref<string | null>(null)
let intervalId: ReturnType<typeof setInterval>

/**
 * Format ISO timestamp to localized string
 */
function formatDate(ts: string): string {
  return new Date(ts).toLocaleString()
}

/**
 * Fetch latest product and tracker events, merge and sort by timestamp desc
 */
async function fetchEvents() {
  loading.value = true
  error.value = null
  try {
    const [pe, te] = await Promise.all([
      http.get<{ results: any[] }>(`/api/product-events/?page_size=20`),
      http.get<{ results: any[] }>(`/api/tracker-events/?page_size=20`),
    ])
    const prodEvents: LiveEvent[] = pe.data.results.map(e => ({
      message_id: e.message_id,
      event_type: e.event_type,
      timestamp: e.timestamp,
      source: 'product',
      ref_id: e.product,
    }))
    const trkEvents: LiveEvent[] = te.data.results.map(e => ({
      message_id: e.message_id,
      event_type: e.event_type,
      timestamp: e.timestamp,
      source: 'tracker',
      ref_id: null,
    }))
    // merge and sort descending
    events.value = [...prodEvents, ...trkEvents]
      .sort((a, b) => new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime())
  } catch (e: any) {
    error.value = e.message || String(e)
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  fetchEvents()
  intervalId = setInterval(fetchEvents, 10000)
})

onBeforeUnmount(() => {
  clearInterval(intervalId)
})
</script>

<template>
  <div class="space-y-2">
    <div v-if="loading" class="text-center">Loading events…</div>
    <div v-else-if="error" class="text-red-500">Error: {{ error }}</div>
    <ul v-else class="divide-y divide-gray-200">
      <li v-for="ev in events" :key="ev.message_id" class="py-2">
        <div class="flex items-center justify-between">
          <div class="flex items-center space-x-2">
            <Badge :variant="ev.source === 'product' ? 'primary' : 'secondary'">
              {{ ev.source === 'product' ? 'Product' : 'Tracker' }}
            </Badge>
            <span class="font-semibold">{{ ev.event_type }}</span>
          </div>
          <span class="text-xs text-muted-foreground">
            {{ formatDate(ev.timestamp) }}
          </span>
        </div>
        <div class="text-sm text-muted-foreground mt-1">
          <span v-if="ev.source === 'product'">Product ID: {{ ev.ref_id }}</span>
          <span v-else>Tracker Event ID: {{ ev.message_id }}</span>
        </div>
      </li>
    </ul>
  </div>
</template>
