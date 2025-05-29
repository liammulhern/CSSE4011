<script setup lang="ts">
import GlobalSearchPopover from '@/components/core/GlobalSearchPopover.vue'
import { Avatar, AvatarImage } from '@/components/ui/avatar'
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuLabel,
  DropdownMenuSeparator,
  DropdownMenuShortcut,
  DropdownMenuTrigger,
} from '@/components/ui/dropdown-menu'
import Breadcrumb from '@/components/ui/Breadcrumb.vue'
import {
  LogOut,
  User,
  Bell,
  Sun,
  MoonStar,
  Menu,
} from 'lucide-vue-next'
import { Button } from '@/components/ui/button'
import { useAppStore } from '@/stores/app'
import { useAuthStore } from '@/stores/auth'
import { useProductNotificationStore } from '@/stores/productnotification'
import { computed, onMounted } from 'vue'
import { logout, isAuthenticated } from '@/services/auth'
import { useRoute, useRouter } from 'vue-router'

const store = useAppStore()
const auth = useAuthStore()
const route = useRoute()
const router = useRouter()
const notificationStore = useProductNotificationStore()

// Load alerts on mount
onMounted(() => {
  notificationStore.fetchAlerts()
})

const toggleMode = () => {
  store.toggleTheme()
}

const user = computed(() => auth.user)
const username = computed(() => auth.user?.username ?? 'Guest')
const useremail = computed(() => auth.user?.email ?? '')

// Count unacknowledged alerts
const unackCount = computed(() => {
  const count = notificationStore.alerts.filter(n => !n.acknowledged_timestamp).length
  return count > 99 ? '99+' : String(count)
})

const goToNotification = () => {
  router.push({ name: 'dashboard', query: { tab: 'events' } })
};

const showBadge = computed(() => parseInt(unackCount.value) > 0 || unackCount.value === '99+')
</script>

<template>
  <nav
    class="flex items-center justify-between h-[64px] border-b-[1px] px-4 fixed z-40 top-0 bg-background/80 backdrop-blur-lg border-b border-border"
    :style="{ width: store.navWidth }">
    <div class="w-24 hidden lg:block">
      <Breadcrumb />
    </div>
    <div class="w-2/5 hidden lg:block">
      <GlobalSearchPopover />
    </div>
    <Button variant="outline" class="p-[6px] w-8 h-8 transition-all duration-200 block lg:hidden"
      :class="store.sidebarExpanded ? 'bg-transparent' : 'dark:bg-white'" @click="store.toggleSidebar()">
      <Menu class="transition-all duration-500 text-black" />
    </Button>
    <div class="flex items-center">
      <!-- Bell icon with badge -->
      <Button variant="outline" class="relative border-0 p-[6px] w-8 h-8" @click="goToNotification">
        <Bell />
        <span v-if="showBadge"
          class="absolute -top-1 -right-1 inline-flex items-center justify-center w-4 h-4 text-[10px] font-bold leading-none rounded-full bg-[hsl(var(--destructive))] text-[hsl(var(--destructive-foreground))]">
          {{ unackCount }}
        </span>
      </Button>

      <Button variant="outline" class="border-0 p-[6px] ml-2 w-8 h-8" @click="toggleMode">
        <Sun v-if="store.isDark" />
        <MoonStar v-else />
      </Button>
      <div class="border-x-[1px] border-gray-300 h-[24px] w-[1px] mx-2"></div>
      <DropdownMenu>
        <DropdownMenuTrigger as-child>
          <Button variant="outline" class="border-0 flex items-center max-w-[200px] w-full justify-start">
            <Avatar class="h-8 w-8">
              <User class="h-4 w-4" />
            </Avatar>
            <span class="ml-2 hidden md:flex justify-start flex-col items-start">
              <p class="mb-0">{{ username }}</p>
              <small class="text-xs text-slate-400 font-light">{{ useremail }}</small>
            </span>
          </Button>
        </DropdownMenuTrigger>
        <DropdownMenuContent class="w-56 relative mr-4" v-if="isAuthenticated()">
          <DropdownMenuLabel>{{ username }}</DropdownMenuLabel>
          <DropdownMenuSeparator />
          <DropdownMenuItem @click="logout()">
            <LogOut class="mr-2 h-4 w-4" />
            <span>Log out</span>
          </DropdownMenuItem>
        </DropdownMenuContent>
      </DropdownMenu>
    </div>
  </nav>
</template>
