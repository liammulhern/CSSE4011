<script setup lang="ts">
import { onMounted, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useProductNotificationStore, type ProductNotification } from '@/stores/productnotification'
import { Card, CardContent } from '@/components/ui/card'
import {
  AlertTriangle,
  Bell,
  Info,
} from 'lucide-vue-next'

// Setup router and store
const router = useRouter()
const notificationStore = useProductNotificationStore()

// Fetch recent notifications on mount
onMounted(() => {
  notificationStore.fetchAlerts()
})

// Compute the 5 most recent unacknowledged alerts
const recentNotifications = computed<ProductNotification[]>(() =>
  notificationStore.alerts
    .filter(n => !n.acknowledged_timestamp)
    .slice(0, 5)
)

// Choose icon based on notification_type
function iconFor(type: string) {
  switch (type) {
    case 'alert': return AlertCircle
    case 'notification': return Info
    default: return Bell
  }
}

// Handle click: navigate to notification detail
function goToNotification(id: number) {
  router.push({ name: 'product_order_index', params: { id: String(id) } })
}
</script>

<template>
  <h2 class="text-lg font-medium pb-4"> Unacknowledged Alerts </h2>
  <div v-for="notif in recentNotifications" :key="notif.id">
    <Card class="cursor-pointer hover:bg-muted px-0" @click="goToNotification(notif.order)">
      <CardContent class="flex flex-col items-start space-y-4 max-h-[150px] overflow-y-auto">
        <!-- Message and order -->
        <div class="flex flex-row items-center space-x-4">
          <div class="flex flex-col">
            <p class="text-sm font-medium flex items-center">
              <AlertTriangle v-if="notif.notication_type == 'alert'" class="w-4 h-4 mr-4" />
              <Bell v-else class="w-4 h-4 mr-4" />
              Order #{{ notif.order }}
            </p>
          </div>
        </div>
        <!-- Timestamp -->
        <div class="text-xs text-muted-foreground">
          {{ new Date(notif.timestamp).toLocaleString() }}
        </div>
        <p class="text-sm text-muted-foreground wrap">{{ notif.message }}</p>
      </CardContent>
    </Card>
  </div>
  <div v-if="recentNotifications.length === 0" class="text-sm text-muted-foreground text-center py-4">
    No recent alerts.
  </div>
</template>
