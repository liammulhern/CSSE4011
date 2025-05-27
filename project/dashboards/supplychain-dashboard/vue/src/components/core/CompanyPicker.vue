<script setup lang="ts">
import { ref, computed, watch } from 'vue'
import { useAuthStore } from '@/stores/auth'
import { useAppStore } from '@/stores/app'
import { Select, SelectTrigger, SelectValue, SelectContent, SelectGroup, SelectItem } from '@/components/ui/select'
import { Avatar, AvatarImage, AvatarFallback } from '@/components/ui/avatar'

// Access authenticated user and app-level store
const auth = useAuthStore()
const app = useAppStore()

// Derive companies from user object (ensure UserSerializer includes `companies`)
interface UserCompany { company: { id: number; name: string; type: string }; is_active: boolean }
const companies = computed<UserCompany[]>(() => auth.user?.companies ?? [])

// Selected company id; initialize from app.store or fallback to first active
const selectedId = ref<number | null>(null)
const initial = app.activeCompanyId ?? companies.value.find(c => c.is_active)?.company.id
selectedId.value = initial ?? null

// Update app state when selection changes
watch(selectedId, (val) => {
  if (val !== null) app.setActiveCompany(val)
})
</script>

<template>
  <Select v-model="selectedId">
    <!-- Trigger button shows avatar + name -->
    <SelectTrigger class="justify-between">
      <SelectValue placeholder="Select Company" />
    </SelectTrigger>

    <SelectContent>
      <SelectGroup>
        <!-- v-for moved to template with key -->
        <template v-for="uc in companies" :key="uc.company.id">
          <SelectItem :value="uc.company.id">
            <Avatar class="mr-2 h-5 w-5">
              <AvatarImage :src="`/api/companies/${uc.company.id}/avatar/`" alt="Company Logo"
                class="grayscale opacity-80" />
              <AvatarFallback>CP</AvatarFallback>
            </Avatar>
            {{ uc.company.name }}
          </SelectItem>
        </template>
      </SelectGroup>
    </SelectContent>
  </Select>
</template>
