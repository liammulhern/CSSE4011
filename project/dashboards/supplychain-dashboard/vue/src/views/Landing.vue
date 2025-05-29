<script setup lang="ts">
import { ref } from 'vue'
import { watchOnce } from '@vueuse/core'
import type { CarouselApi } from '@/components/ui/carousel'
import { Card, CardHeader, CardTitle, CardContent } from '@/components/ui/card'
import { Button } from '@/components/ui/button'
import { Sun, MoonStar } from 'lucide-vue-next'
import { useRouter } from 'vue-router'
import { useAppStore } from '@/stores/app'
import Autoplay from 'embla-carousel-autoplay'
import { Carousel, CarouselContent, CarouselItem } from '@/components/ui/carousel'

// Import backgrounds
import bg1 from '@/assets/images/landingpage/landingpage-bg-1.jpg'
import bg2 from '@/assets/images/landingpage/landingpage-bg-2.jpg'
import bg3 from '@/assets/images/landingpage/landingpage-bg-3.jpg'
import bg4 from '@/assets/images/landingpage/landingpage-bg-4.jpg'

const router = useRouter()
const store = useAppStore()
function goLogin() { router.push({ name: 'dashboard' }) }
function goGetStarted() { goLogin() }
const toggleMode = () => store.toggleTheme()

// Carousel images and autoplay
const images = [bg1, bg2, bg3, bg4]
const autoplay = Autoplay({ delay: 10000, stopOnMouseEnter: false, stopOnInteraction: false })

// Carousel state
const api = ref<CarouselApi>()
const current = ref(0)
const total = images.length

function setApi(val: CarouselApi) {
  api.value = val
  // Initialize current
  current.value = val.selectedScrollSnap()
  // Update on select
  val.on('select', () => { current.value = val.selectedScrollSnap() })
}
</script>

<template>
  <div class="min-h-screen flex flex-col bg-background text-foreground">
    <!-- Header -->
    <header class="flex items-center justify-between px-6 py-4">
      <h2 class="text-2xl text-primary font-semibold flex items-center">
        <span class="mr-2 flex items-center">
          <Icon name="Package" />
        </span>PathLedger
      </h2>
      <div class="flex items-center">
        <Button variant="outline" class="border-0 p-[6px] ml-2 w-8 h-8" @click="toggleMode">
          <Sun v-if="store.isDark" />
          <MoonStar v-else />
        </Button>
        <div class="border-x-[1px] border-gray-300 h-6 w-px mx-4"></div>
        <Button @click="goLogin">Login</Button>
      </div>
    </header>

    <!-- Hero -->
    <section class="flex-1 relative overflow-hidden">
      <Carousel :plugins="[autoplay]" class="absolute inset-0 h-full w-full" loop @init-api="setApi">
        <CarouselContent class="absolute inset-0">
          <CarouselItem v-for="src in images" :key="src">
            <div class="h-full w-full bg-cover bg-center filter brightness-50 saturate-0 transform animate-slow-zoom"
              :style="{ backgroundImage: `url(${src})` }"></div>
          </CarouselItem>
        </CarouselContent>
      </Carousel>

      <!-- Tint overlay -->
      <div class="absolute inset-0 bg-accent opacity-30"></div>

      <!-- Indicators -->
      <div class="absolute bottom-6 left-1/2 transform -translate-x-1/2 z-20 flex space-x-2">
        <button v-for="(src, idx) in images" :key="idx"
          class="w-3 h-3 rounded-full border border-[hsl(var(--primary-foreground))]"
          :class="idx === current ? 'bg-[hsl(var(--primary-foreground))]' : ''" @click="api?.scrollTo(idx)"></button>
      </div>

      <!-- Content -->
      <div class="absolute inset-0 z-10 flex flex-col items-center justify-center text-center px-6">
        <h2 class="text-5xl font-extrabold mb-4 text-primary-foreground drop-shadow-lg">
          Secure your supply chain with immutable ledgers
        </h2>
        <p class="max-w-lg text-lg text-primary-foreground mb-8">
          Trace every product’s journey end-to-end using IoT-backed blockchain proofs.
        </p>
        <Button class="px-8 py-3 text-primary-foreground bg-primary" @click="goGetStarted()">
          Get Started
        </Button>
      </div>
    </section>

    <!-- Features -->
    <section class="bg-muted py-16">
      <div class="max-w-5xl mx-auto grid grid-cols-1 md:grid-cols-3 gap-6 px-6">
        <Card>
          <CardHeader>
            <CardTitle>Immutable Records</CardTitle>
          </CardHeader>
          <CardContent class="text-muted-foreground">Every movement is cryptographically signed and stored on-chain.
          </CardContent>
        </Card>
        <Card>
          <CardHeader>
            <CardTitle>Real-Time Visibility</CardTitle>
          </CardHeader>
          <CardContent class="text-muted-foreground">Monitor status, location and condition of assets live.
          </CardContent>
        </Card>
        <Card>
          <CardHeader>
            <CardTitle>Easy Integration</CardTitle>
          </CardHeader>
          <CardContent class="text-muted-foreground">Plug into existing sensors, gateways and blockchain nodes.
          </CardContent>
        </Card>
      </div>
    </section>

    <!-- Footer -->
    <footer class="py-6 text-center text-muted-foreground">
      © {{ new Date().getFullYear() }} PathLedger — All rights reserved.
    </footer>
  </div>
</template>
